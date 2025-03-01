#ifndef FOC_H
#define FOC_H


struct vec2 {
  float d[2];
  float &a() {return d[0];}
  float &b() {return d[1];}
  float a() const {return d[0];}
  float b() const {return d[1];}
};

struct vec3 {
  float d[3];
  float &a() {return d[0];}
  float &b() {return d[1];}
  float &c() {return d[2];}
  float a() const {return d[0];}
  float b() const {return d[1];}
  float c() const {return d[2];}
};


#define CCM_FUNC __attribute__((section(".ram4_init.code")))
#define CCM_DATA __attribute__((section(".ram4_init.no_init")))

static void clarke(const float a, const float b, float &alpha, float &beta)
{
    constexpr float beta_scale = 1.f/sqrtf(3.f);
    alpha = a;
    beta = (a + 2.f * b) * beta_scale;
}

static void clarke(const float a, const float b, const float c, float &alpha, float &beta)
{
    constexpr float alpha_scale = 1.f/3.f;
    constexpr float beta_scale = 1.f/sqrtf(3.f);
    alpha = (a * 2.0f - (b + c)) * alpha_scale;
    beta = (b - c) * beta_scale;
}
#include <math.h>
static const float sintab[256] = {
    0.000000, 0.006136, 0.012272, 0.018407, 0.024541, 0.030675, 0.036807, 0.042938,
    0.049068, 0.055195, 0.061321, 0.067444, 0.073565, 0.079682, 0.085797, 0.091909,
    0.098017, 0.104122, 0.110222, 0.116319, 0.122411, 0.128498, 0.134581, 0.140658,
    0.146730, 0.152797, 0.158858, 0.164913, 0.170962, 0.177004, 0.183040, 0.189069,
    0.195090, 0.201105, 0.207111, 0.213110, 0.219101, 0.225084, 0.231058, 0.237024,
    0.242980, 0.248928, 0.254866, 0.260794, 0.266713, 0.272621, 0.278520, 0.284408,
    0.290285, 0.296151, 0.302006, 0.307850, 0.313682, 0.319502, 0.325310, 0.331106,
    0.336890, 0.342661, 0.348419, 0.354164, 0.359895, 0.365613, 0.371317, 0.377007,
    0.382683, 0.388345, 0.393992, 0.399624, 0.405241, 0.410843, 0.416430, 0.422000,
    0.427555, 0.433094, 0.438616, 0.444122, 0.449611, 0.455084, 0.460539, 0.465977,
    0.471397, 0.476799, 0.482184, 0.487550, 0.492898, 0.498228, 0.503538, 0.508830,
    0.514103, 0.519356, 0.524590, 0.529804, 0.534998, 0.540172, 0.545325, 0.550458,
    0.555570, 0.560662, 0.565732, 0.570781, 0.575808, 0.580814, 0.585798, 0.590760,
    0.595699, 0.600616, 0.605511, 0.610383, 0.615232, 0.620057, 0.624860, 0.629638,
    0.634393, 0.639124, 0.643832, 0.648514, 0.653173, 0.657807, 0.662416, 0.667000,
    0.671559, 0.676093, 0.680601, 0.685084, 0.689541, 0.693971, 0.698376, 0.702755,
    0.707107, 0.711432, 0.715731, 0.720003, 0.724247, 0.728464, 0.732654, 0.736817,
    0.740951, 0.745058, 0.749136, 0.753187, 0.757209, 0.761202, 0.765167, 0.769103,
    0.773010, 0.776888, 0.780737, 0.784557, 0.788346, 0.792107, 0.795837, 0.799537,
    0.803208, 0.806848, 0.810457, 0.814036, 0.817585, 0.821102, 0.824589, 0.828045,
    0.831470, 0.834863, 0.838225, 0.841555, 0.844854, 0.848120, 0.851355, 0.854558,
    0.857729, 0.860867, 0.863973, 0.867046, 0.870087, 0.873095, 0.876070, 0.879012,
    0.881921, 0.884797, 0.887640, 0.890449, 0.893224, 0.895966, 0.898674, 0.901349,
    0.903989, 0.906596, 0.909168, 0.911706, 0.914210, 0.916679, 0.919114, 0.921514,
    0.923880, 0.926210, 0.928506, 0.930767, 0.932993, 0.935184, 0.937339, 0.939459,
    0.941544, 0.943593, 0.945607, 0.947586, 0.949528, 0.951435, 0.953306, 0.955141,
    0.956940, 0.958703, 0.960431, 0.962121, 0.963776, 0.965394, 0.966976, 0.968522,
    0.970031, 0.971504, 0.972940, 0.974339, 0.975702, 0.977028, 0.978317, 0.979570,
    0.980785, 0.981964, 0.983105, 0.984210, 0.985278, 0.986308, 0.987301, 0.988258,
    0.989177, 0.990058, 0.990903, 0.991710, 0.992480, 0.993212, 0.993907, 0.994565,
    0.995185, 0.995767, 0.996313, 0.996820, 0.997290, 0.997723, 0.998118, 0.998476,
    0.998795, 0.999078, 0.999322, 0.999529, 0.999699, 0.999831, 0.999925, 0.999981,
};

#define SINTABSCALE ((1024.0f / 2.0f / M_PI))

CCM_FUNC void sincos(const float angle, float &a, float &b)
{
    unsigned x = 0x3ff & abs(int(angle * SINTABSCALE + 0.5f));
    unsigned quad = x >> 8;
    x &= 0xff;
    switch (quad) {
    case 0:
        a = sintab[x];
        b = sintab[255 - x];
        break;
    case 1:
        a = sintab[255 - x];
        b = -sintab[x];
        break;
    case 2:
        a = -sintab[x];
        b = -sintab[255 - x];
        break;
    case 3:
        a = -sintab[255 - x];
        b = sintab[x];
        break;
    }
    if (angle < 0)
        a = -a;
}

struct PI {
    float limit;
    float sum;
    float out;
    PI() : limit(0.0f), sum(0.0f), out(0.0f) {}
    float run(const vec2 &Kab, const float error, const float ff);
    float run(const vec2 &Kab, const float error);
};

CCM_FUNC float PI::run(const vec2 &Kab, const float error, const float ff) {
    out = Kab.a() * error;
    float x = Kab.b() * out;
    float mx = fmax(limit - out, 0.0f);
    float mn = fmin(-limit - out, 0.0f);
    sum += x;
    sum = fmax(fmin(sum, mx), mn);
    out += sum + ff;
    out = fmax(fmin(out, mx), mn);
    return out;
}
CCM_FUNC float PI::run(const vec2 &Kab, const float error) {
    out = Kab.a() * error;
    float x = Kab.b() * out;
    float mx = fmax(limit - out, 0.0f);
    float mn = fmin(-limit - out, 0.0f);
    sum += x;
    sum = fmax(fmin(sum, mx), mn);
    out += sum;
    out = fmax(fmin(out, mx), mn);
    return out;
}

const float Ls = 14.7e-6f;
const float Rs = 80e-3f;
const float Ke = 0.00152f;
const float Ts = 2.0f/FOC_PWM_FREQ;

struct AngleEstimator {
    float Vrsa, Vrsb;
    PI bemfa, bemfb, omega;
    vec2 iK, oK;
    float angle;
    float angle_err;
    float sine, cosine;
    float F;
    AngleEstimator() : angle(0.0f) {
       F = expf(-Rs * Ts / Ls);
       iK.a() = Ls * M_PI /(10.0f * Ts);
       iK.b() = 0.1f * (Rs / Ls) * Ts;
       Vrsa = Vrsb = 0.0f;
       oK.a() = 1e4f;
       oK.b() = 1e-3f;
       omega.limit = 15000.0f;
       bemfa.limit = bemfb.limit = 24.0f;
       sincos(angle, sine, cosine);
    }
    void run(const float Va, const float Vb, const float Ia, const float Ib);
};

CCM_FUNC void AngleEstimator::run(const float Va, const float Vb, const float Ia, const float Ib) {
    Vrsa = F * Vrsa + (1.0 - F) * (Va - bemfa.out);
    bemfa.run(iK, Vrsa / Rs - Ia);
    Vrsb = F * Vrsb + (1.0 - F) * (Vb - bemfb.out);
    bemfb.run(iK, Vrsb / Rs - Ib);
    angle_err = bemfa.out * sine + bemfb.out * cosine;
    omega.run(oK, -angle_err);
    angle += omega.out * Ts;
    sincos(angle, sine, cosine);
}

static void park(const float sine, const float cosine, const float alpha, const float beta, float &d, float &q)
{
   d = alpha * cosine + beta * sine;
   q = beta * cosine - alpha * sine;
}

static void ipark(const float sine, const float cosine, const float d, const float q, float &alpha, float &beta)
{
    alpha = d * cosine - q * sine;
    beta = q * cosine + d * sine;
}

static void iclarke(const float alpha, const float beta, float &a, float &b, float &c)
{
    constexpr float sqrt3 = sqrt(3.0);
    a = alpha;
    b = (sqrt3 * beta - alpha) / 2.0;
    c = -(sqrt3 * beta + alpha) / 2.0;
}

static void svgen(const float alpha, const float beta, float &a, float &b, float &c, const float Vbus)
{
    const float scale = 2.0 / Vbus;
    iclarke(alpha, beta, a, b, c);
#if 1
    const float mn = fmin(a, fmin(b, c));
    const float mx = fmax(a, fmax(b, c));
    const float off = (mn + mx) * .5;
    a -= off;
    b -= off;
    c -= off;
#endif
    a *= scale;
    b *= scale;
    c *= scale;
}

CCM_FUNC static unsigned pwm_saturate(const float x) {
    float y = FOC_PWM_PERIOD * (x + 1.0f) * 0.5f;
    int c = int(y);
    if (c < 0)
        c = 0;
    if (c >= FOC_PWM_PERIOD)
        c = FOC_PWM_PERIOD - 1;
    return c;
}

CCM_FUNC void pwm_set_duty(const float a, const float b, const float c) {
    pwm_set_channel(0, pwm_saturate(a));
    pwm_set_channel(1, pwm_saturate(b));
    pwm_set_channel(2, pwm_saturate(c));
}
enum FOC_STATE {

};
struct FOC {
    thread_t *thread;
    AngleEstimator aest;
    PI id_pi, iq_pi;
    vec2 iK;
    float id_req, iq_req;
    float sine, cosine;
    void foc_hot();
    FOC() : thread(0) {}
    void init() {
        iK.a() = Ls * M_PI /(10 * Ts);
        iK.b() = (Rs / Ls) * Ts;
        id_pi.limit = iq_pi.limit = 30.0f;
        sine = 0.0f;
        cosine = 1.0f;
        iq_req = id_req = 0.0f;
    }
    void fsm() {

    }
};

CCM_DATA FOC foc;
CCM_DATA ADC adc;

static THD_WORKING_AREA(waThread1, 512);

CCM_FUNC void FOC::foc_hot() {
    while (true) {
        chEvtWaitAny(ALL_EVENTS);
        palSetLine(LINE_LED_RED);
        float ia, ib;
        clarke(adc.Ia(), adc.Ib(), ia, ib);
        float va, vb;
        clarke(adc.Va(), adc.Vb(), adc.Vc(), va, vb);
        aest.run(va, vb, ia, ib);
        float id, iq;
        park(sine, cosine, ia, ib, id, iq);
        float omega = 0.0f;
        float vd, vq;
        vd = id_pi.run(iK, id_req - id, omega * Ls * iq);
        iq_pi.limit = sqrt(id_pi.limit * id_pi.limit - vd * vd);
        vq = iq_pi.run(iK, iq_req - iq, omega * (Ls * id + Ke));
        ipark(sine, cosine, vd, vq, va, vb);
        float vc;
        svgen(vd, vq, va, vb, vc, adc.Vbus());
        //pwm_set_duty(va, vb, vc);
        palClearLine(LINE_LED_RED);
    }
}
CCM_FUNC static THD_FUNCTION(Thread1, arg) {
  (void)arg;
  chRegSetThreadName("foc");
  pwm_set_channel(3, FOC_PWM_PERIOD - 2);
  adc.init();
  foc.init();
#if 1
  foc.foc_hot();
#endif
}

#define EVT_ADC_COMPLETE EVENT_MASK(0)

extern "C" {
CCM_FUNC OSAL_IRQ_HANDLER(STM32_ADC1_HANDLER) {
  OSAL_IRQ_PROLOGUE();
  uint32_t isr = ADC1->ISR;
  ADC1->ISR = isr;
  if (isr & ADC_ISR_JEOS) {
    chSysLockFromISR();
    if (foc.thread)
        chEvtSignalI(foc.thread, EVT_ADC_COMPLETE);
    chSysUnlockFromISR();
  }
  OSAL_IRQ_EPILOGUE();
}
}

#endif // FOC_H
