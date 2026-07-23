#include <map>
#include <fstream>
#include <numeric>
#include <cmath>
#include <cstring>
#include <cstdio>

#include "tts/kokoro.hpp"
#include "frontend/frontend_interface.hpp"
#include "frontend/kokoro_frontend.hpp"
#include "utils/logger.h"
#include "utils/string_utils.hpp"
#include "ax_model_runner/ax_model_runner.hpp"
#include "onnxruntime_cxx_api.h"

#define MAX_SEQ_LEN     96
#define STYLE_DIM       256
#define T_PAD           192
#define F0_LEN          384
#define VOICE_HEAD_DIM  128

static std::vector<float> sigmoid_vec(const std::vector<float>& x) {
    std::vector<float> r(x.size());
    for (size_t i = 0; i < x.size(); i++) r[i] = 1.0f / (1.0f + expf(-x[i]));
    return r;
}
static void transpose(const float* A, float* At, int m, int k) {
    for (int i = 0; i < m; i++) for (int j = 0; j < k; j++) At[j*m+i] = A[i*k+j];
}
static void matmul(const float* A, const float* B, float* C, int m, int k, int n) {
    for (int i = 0; i < m; i++) for (int j = 0; j < n; j++) { float s=0; for (int p=0;p<k;p++) s+=A[i*k+p]*B[p*n+j]; C[i*n+j]=s; }
}

class Kokoro::Impl {
public:
    ~Impl() { uninit(); }
    bool init(AX_TTS_TYPE_E, AX_TTS_INIT_CONFIG* cfg) {
        std::string mp(cfg->model_path); model_path_ = mp;
        return load_models_(mp);
    }
    void uninit() { enc_.unload_model(); f0n_.unload_model(); dec_.unload_model(); har_sess_.release(); istft_sess_.release(); }
    bool run(const std::vector<int>& input_ids, AX_TTS_RUN_CONFIG* cfg, AX_TTS_AUDIO** audio) {
        if (!cfg->voice) return false;
        std::string vn(cfg->voice);
        if (vn != voice_name_) { if (!load_voice_(model_path_, vn)) return false; voice_name_ = vn; }
        std::vector<float> ad;
        if (!run_models_(input_ids, cfg->speed, ad)) return false;
        *audio = (AX_TTS_AUDIO*)malloc(sizeof(AX_TTS_AUDIO)+sizeof(float)*ad.size());
        auto* a=*audio; a->channels=1; a->num_samples=(int)ad.size(); a->sample_rate=cfg->sample_rate;
        std::memcpy(a->data, ad.data(), sizeof(float)*ad.size());
        return true;
    }
private:
    AxModelRunner enc_,f0n_,dec_;
    Ort::Env ort_env_{ORT_LOGGING_LEVEL_WARNING,"Kokoro"};
    Ort::Session har_sess_{nullptr},istft_sess_{nullptr};
    std::string voice_name_,model_path_;
    std::vector<float> voice_tensor_,d_buf_,t_en_buf_,dur_buf_,f0_buf_,n_buf_,dec_buf_;
    std::vector<int> d_shape_,f0_shape_,dec_shape_;

    bool load_models_(const std::string& mp) {
        std::string ep=mp+"/kokoro_enc_axera.axmodel"; if(enc_.load_model(ep.c_str())!=0){ALOGE("enc:%s",ep.c_str());return false;}
        std::string fp=mp+"/kokoro_f0n.axmodel"; if(f0n_.load_model(fp.c_str())!=0){ALOGE("f0n:%s",fp.c_str());return false;}
        std::string dp=mp+"/kokoro_dec.axmodel"; if(dec_.load_model(dp.c_str())!=0){ALOGE("dec:%s",dp.c_str());return false;}
        Ort::SessionOptions so; so.SetIntraOpNumThreads(1);
        har_sess_=Ort::Session(ort_env_,(mp+"/kokoro_har_noup.onnx").c_str(),so);
        istft_sess_=Ort::Session(ort_env_,(mp+"/kokoro_istft.onnx").c_str(),so);
        d_buf_.resize(enc_.get_output_size(0)/sizeof(float)); t_en_buf_.resize(enc_.get_output_size(1)/sizeof(float));
        dur_buf_.resize(enc_.get_output_size(2)/sizeof(float)); f0_buf_.resize(f0n_.get_output_size(0)/sizeof(float));
        n_buf_.resize(f0n_.get_output_size(1)/sizeof(float)); dec_buf_.resize(dec_.get_output_size(0)/sizeof(float));
        d_shape_=enc_.get_output_shape(0); f0_shape_=f0n_.get_output_shape(0); dec_shape_=dec_.get_output_shape(0);
        return true;
    }
    bool load_voice_(const std::string& mp,const std::string& vn){
        std::string p=mp+"/voices/"+vn+".bin"; FILE* f=fopen(p.c_str(),"rb");
        if(!f){ALOGE("voice %s",p.c_str());return false;}
        voice_tensor_.resize(STYLE_DIM);
        if(fread(voice_tensor_.data(),sizeof(float),STYLE_DIM,f)!=STYLE_DIM){fclose(f);return false;}
        fclose(f); return true;
    }
    bool run_models_(std::vector<int> input_ids,float speed,std::vector<float>& audio){
        int al=(int)input_ids.size(); if(al>MAX_SEQ_LEN){input_ids.resize(MAX_SEQ_LEN);al=MAX_SEQ_LEN;}
        input_ids.resize(MAX_SEQ_LEN,0);
        std::vector<float> sh(voice_tensor_.begin(),voice_tensor_.begin()+VOICE_HEAD_DIM);
        std::vector<float> st(voice_tensor_.begin()+VOICE_HEAD_DIM,voice_tensor_.end());
        // 1. Encoder
        std::vector<void*> ei{(void*)input_ids.data(),(void*)st.data()};
        std::vector<void*> eo{(void*)d_buf_.data(),(void*)t_en_buf_.data(),(void*)dur_buf_.data()};
        enc_.set_inputs(ei); if(enc_.run()!=0)return false; enc_.get_outputs(eo);
        // 2. Duration+Align
        int tf; std::vector<int> pd; process_duration_(dur_buf_,al,speed,pd,tf);
        std::vector<float> dT(640*96); transpose(d_buf_.data(),dT.data(),96,640);
        std::vector<float> aln(MAX_SEQ_LEN*tf,0);
        for(int i=0,c=0;i<MAX_SEQ_LEN;i++)for(int r=0;r<pd[i];r++)if(c<tf)aln[i*tf+c++]=1;
        std::vector<float> en_raw(640*tf),en_buf(640*T_PAD,0);
        matmul(dT.data(),aln.data(),en_raw.data(),640,96,tf);
        for(int i=0;i<640;i++)std::memcpy(&en_buf[i*T_PAD],&en_raw[i*tf],tf*4);
        std::vector<float> asr_raw(512*tf),asr_buf(512*T_PAD,0);
        matmul(t_en_buf_.data(),aln.data(),asr_raw.data(),512,96,tf);
        for(int i=0;i<512;i++)std::memcpy(&asr_buf[i*T_PAD],&asr_raw[i*tf],tf*4);
        // 3. F0N
        std::vector<void*> fi{(void*)en_buf.data(),(void*)st.data()};
        std::vector<void*> fo{(void*)f0_buf_.data(),(void*)n_buf_.data()};
        f0n_.set_inputs(fi); if(f0n_.run()!=0)return false; f0n_.get_outputs(fo);
        // 4. HAR
        std::vector<float> f0_up(115200);
        for(int i=0;i<F0_LEN;i++)for(int j=0;j<300;j++)f0_up[i*300+j]=f0_buf_[i];
        int64_t fs[]={1,115200};
        auto mem=Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator,OrtMemTypeCPU);
        std::vector<Ort::Value> hi;
        hi.push_back(Ort::Value::CreateTensor<float>(mem,f0_up.data(),115200,fs,2));
        const char* hin[]={"f0_up"},*hout[]={"har"};
        auto ho=har_sess_.Run(Ort::RunOptions{nullptr},hin,hi.data(),1,hout,1);
        std::vector<float> har(ho.front().GetTensorTypeAndShapeInfo().GetElementCount());
        std::memcpy(har.data(),ho.front().GetTensorMutableData<float>(),har.size()*4);
        // 5. Decoder
        std::vector<void*> di{(void*)asr_buf.data(),(void*)f0_buf_.data(),(void*)n_buf_.data(),(void*)sh.data(),(void*)har.data()};
        dec_.set_inputs(di); if(dec_.run()!=0)return false; dec_.get_output(0,dec_buf_.data());
        // 6. ISTFT
        int64_t rs[]={1,(int64_t)dec_shape_[1],(int64_t)dec_shape_[2]};
        std::vector<Ort::Value> ii;
        ii.push_back(Ort::Value::CreateTensor<float>(mem,dec_buf_.data(),dec_buf_.size(),rs,3));
        const char* iin[]={"raw_x"},*iout[]={"waveform"};
        auto io=istft_sess_.Run(Ort::RunOptions{nullptr},iin,ii.data(),1,iout,1);
        audio.resize(io.front().GetTensorTypeAndShapeInfo().GetElementCount());
        std::memcpy(audio.data(),io.front().GetTensorMutableData<float>(),audio.size()*4);
        int ts=(int)(tf*300.0f*224.0f/151.0f);
        if(ts<(int)audio.size())audio.resize(ts);
        return true;
    }
    void process_duration_(const std::vector<float>& dur,int al,float speed,std::vector<int>& pd,int& tf){
        auto ds=sigmoid_vec(dur); int D50=50;
        pd.resize(MAX_SEQ_LEN,0); int total=0,fixed=MAX_SEQ_LEN*2;
        for(int i=0;i<al;i++){float s=0;for(int j=0;j<D50;j++)s+=ds[i*D50+j];pd[i]=std::max(1,(int)roundf(s/speed));total+=pd[i];}
        int diff=fixed-total,pl=MAX_SEQ_LEN-al;
        if(diff>0&&pl>0){int e=diff/pl,r=diff%pl;
            for(int i=al;i<MAX_SEQ_LEN;i++)pd[i]=e;
            for(int i=al;i<al+r;i++)pd[i]++;}
        else if(diff<0){for(int d=abs(diff);d>0;){int mi=0;float mv=-1;
            for(int i=0;i<al;i++)if(pd[i]>1&&(float)pd[i]>mv){mv=(float)pd[i];mi=i;}
            if(mv<=1||mi>=al)break;pd[mi]--;d--;}}
        tf=std::accumulate(pd.begin(),pd.end(),0);
    }
};

Kokoro::Kokoro():impl_(std::make_unique<Impl>()){}
Kokoro::~Kokoro(){uninit();}
void Kokoro::uninit(){impl_.reset();}

bool Kokoro::init(AX_TTS_TYPE_E tts_type,AX_TTS_INIT_CONFIG* cfg){
    std::string vp=std::string(cfg->model_path)+"/vocab.txt";
    FILE* f=fopen(vp.c_str(),"r"); if(!f){ALOGE("vocab %s",vp.c_str());return false;}
    char line[256];
    while(fgets(line,sizeof(line),f)){char* tab=strchr(line,'\t');if(tab){*tab=0;vocab_[line]=atoi(tab+1);}}
    fclose(f);
    // Init frontend for text-to-phoneme conversion
    frontend_ = std::make_shared<KokoroFrontend>();
    if (!frontend_->init(cfg)) { ALOGE("frontend init failed"); return false; }
    return impl_->init(tts_type,cfg);
}

bool Kokoro::run(const std::string& text,AX_TTS_RUN_CONFIG* cfg,AX_TTS_AUDIO** audio){
    int err=0;
    std::string lang(cfg->language);
    auto ids = frontend_->run(text,lang,vocab_,err);
    if(err!=0||ids.empty()){ALOGE("frontend run failed err=%d",err);return false;}
    return impl_->run(ids,cfg,audio);
}
