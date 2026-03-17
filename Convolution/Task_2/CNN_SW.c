
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "xtime_l.h"

#include "platform.h"
#include <math.h>


#include "conv1_weights.h"
#include "conv1_bias.h"
#include "conv2_weights.h"
#include "conv2_bias.h"
#include "dense1_weights.h"
#include "dense1_bias.h"
#include "dense2_weights.h"
#include "dense2_bias.h"
#include "dense3_weights.h"
#include "dense3_bias.h"


/* ---------------------- Model dimensions ---------------------- */

#define IN_H 49
#define IN_W 10
#define IN_C 1

#define C1_KH 10
#define C1_KW 4
#define C1_OC 28
#define C1_SH 1
#define C1_SW 1
#define C1_OH (((IN_H - C1_KH)/C1_SH) + 1)
#define C1_OW (((IN_W - C1_KW)/C1_SW) + 1)

#define C2_KH 10
#define C2_KW 4
#define C2_IC C1_OC
#define C2_OC 30
#define C2_SH 2
#define C2_SW 1
#define C2_OH (((C1_OH - C2_KH)/C2_SH) + 1)
#define C2_OW (((C1_OW - C2_KW)/C2_SW) + 1)

#define P_KH 2
#define P_KW 4
#define P_SH 2
#define P_SW 4
#define P_OH (((C2_OH - P_KH)/P_SH) + 1)
#define P_OW (((C2_OW - P_KW)/P_SW) + 1)

#define FLAT_LEN (P_OH * P_OW * C2_OC)

#define D1_OUT 16
#define D2_OUT 128
#define D3_OUT 12



#define CONV1_SHIFT 1
#define CONV2_SHIFT 4
#define DENSE1_SHIFT 4
#define DENSE2_SHIFT 5
#define DENSE3_SHIFT 2
#define CONV1_MULT 1158164864
#define CONV1_ZP -128
#define CONV2_MULT 1105153664
#define CONV2_ZP -128
#define DENSE1_MULT 1440123776
#define DENSE2_MULT 1417451776
#define DENSE3_MULT 1742473984
#define DENSE1_ZP -9
#define DENSE2_ZP -128
#define DENSE3_ZP 34

#define NUM_CLASSES 12

#define INPUT_SCALE 1.07421875f
#define INPUT_ZERO  102

void quantize_input(int8_t mfcc[IN_H][IN_W], int8_t q[IN_H][IN_W])
{
    int min = 127;
    int max = -128;

    for (int i = 0; i < IN_H; i++)
    {
        for (int j = 0; j < IN_W; j++)
        {
            float v = ((float)mfcc[i][j] / INPUT_SCALE) + INPUT_ZERO;

            int32_t qi = (int32_t)(v + (v >= 0 ? 0.5f : -0.5f));

            if(qi > 127) qi = 127;
            if(qi < -128) qi = -128;

            q[i][j] = (int8_t)qi;

            if(q[i][j] < min) min = q[i][j];
            if(q[i][j] > max) max = q[i][j];
        }
    }

    printf("Input quantized range: %d .. %d\n", min, max);
}
/* ---------------------- Buffers ---------------------- */

static int8_t input_x[IN_H][IN_W];
static int8_t act1[C1_OH][C1_OW][C1_OC];
static int8_t act2[C2_OH][C2_OW][C2_OC];
static int8_t actp[P_OH][P_OW][C2_OC];

static int8_t flat[FLAT_LEN];

static int8_t d1_out[D1_OUT];
static int8_t d2_out[D2_OUT];

static int32_t logits[D3_OUT];

/* ---------------------- Labels ---------------------- */

static const char *labels[NUM_CLASSES] = {
"silence","unknown","yes","no","up","down",
"left","right","on","off","stop","go"
};

/* ---------------------- Helpers ---------------------- */
static inline int8_t sat_i8_relu_shift(int32_t x, int32_t multiplier, int8_t zero_point, int shift)
{
    /* Q31 multiplication */
    x = (int32_t)(((int64_t)x * multiplier) >> 31);

    /* rounding */
    if(shift > 0)
        x = (x + (1 << (shift - 1))) >> shift;

    /* add zero point */
    x += zero_point;

    /* ReLU clamp */
    if(x < 0) x = 0;
    if(x > 127) x = 127;

    return (int8_t)x;
}



static void conv1_layer_sw(void)
{
    int oh,ow,oc,kh,kw;
    int32_t acc;

    xil_printf("Running Conv1 SW\r\n");

    for(oc=0; oc<C1_OC; oc++)
    for(oh=0; oh<C1_OH; oh++)
    for(ow=0; ow<C1_OW; ow++)
    {
        acc = conv1_b[oc];

        int base = oc*(C1_KH*C1_KW);

        for(kh=0; kh<C1_KH; kh++)
        for(kw=0; kw<C1_KW; kw++)
        {
            int ih = oh + kh;
            int iw = ow + kw;

            int index = base + kh*C1_KW + kw;

            acc += (input_x[ih][iw] - INPUT_ZERO) * conv1_w[index];
        }

        act1[oh][ow][oc] = sat_i8_relu_shift(acc,CONV1_MULT, CONV1_ZP,CONV1_SHIFT);
        //printf("\r\nact1[oh][ow][oc] %d\r\n",act1[oh][ow][oc]);
    }
    int min_val = 127, max_val = -128;
    for (int oh = 0; oh < C1_OH; oh++)
        for (int ow = 0; ow < C1_OW; ow++)
            for (int oc = 0; oc < C1_OC; oc++) {
                if (act1[oh][ow][oc] < min_val) min_val = act1[oh][ow][oc];
                if (act1[oh][ow][oc] > max_val) max_val = act1[oh][ow][oc];
            }
    printf("Conv1 act1 range: %d .. %d\n", min_val, max_val);
}

static void conv2_layer_sw(void)
{
    int oh,ow,oc,kh,kw,ic;
    int32_t acc;

    xil_printf("Running Conv2 SW\r\n");

    for(oc=0; oc<C2_OC; oc++)
    {
        int base_oc = oc*(C2_IC*C2_KH*C2_KW);

        for(oh=0; oh<C2_OH; oh++)
        for(ow=0; ow<C2_OW; ow++)
        {
            acc = conv2_b[oc];

            for(ic=0; ic<C2_IC; ic++)
            {
                int base_ic = base_oc + ic*(C2_KH*C2_KW);

                for(kh=0; kh<C2_KH; kh++)
                for(kw=0; kw<C2_KW; kw++)
                {
                    int ih = oh*C2_SH + kh;
                    int iw = ow*C2_SW + kw;

                    int index = base_ic + kh*C2_KW + kw;

                    acc += act1[ih][iw][ic] * conv2_w[index];
                }
            }

            act2[oh][ow][oc] = sat_i8_relu_shift(acc,CONV2_MULT, CONV2_ZP, CONV2_SHIFT);
        }
    }
    int min_val = 127, max_val = -128;
    for (int oh = 0; oh < C2_OH; oh++)
        for (int ow = 0; ow < C2_OW; ow++)
            for (int oc = 0; oc < C2_OC; oc++) {
                if (act2[oh][ow][oc] < min_val) min_val = act2[oh][ow][oc];
                if (act2[oh][ow][oc] > max_val) max_val = act2[oh][ow][oc];
            }
    printf("Conv2 act2 range: %d .. %d\n", min_val, max_val);
}

static void maxpool_2x4_s2x4(void)
{
    int oh,ow,c,kh,kw;

    for(oh=0;oh<P_OH;oh++)
    for(ow=0;ow<P_OW;ow++)
    for(c=0;c<C2_OC;c++)
    {
        int maxv=-128;

        for(kh=0;kh<P_KH;kh++)
        for(kw=0;kw<P_KW;kw++)
        {
            int ih=oh*P_SH+kh;
            int iw=ow*P_SW+kw;

            if(act2[ih][iw][c]>maxv)
                maxv=act2[ih][iw][c];
        }

        actp[oh][ow][c]=(int8_t)maxv;
    }
}

static void flatten_pooled(void)
{
    int h,w,c;
    int idx=0;

    for(h=0;h<P_OH;h++)
    for(w=0;w<P_OW;w++)
    for(c=0;c<C2_OC;c++)
        flat[idx++]=actp[h][w][c];
}

static void dense1_layer(void)
{
    int o,i;
    int32_t acc;

    for(o=0;o<D1_OUT;o++)
    {
        acc=dense1_b[o];

        int base=o*FLAT_LEN;

        for(i=0;i<FLAT_LEN;i++)
            acc+=flat[i]*dense1_w[base+i];

        d1_out[o]=sat_i8_relu_shift(acc,DENSE1_MULT, DENSE1_ZP, DENSE1_SHIFT);
    }
}

static void dense2_layer(void)
{
    int o,i;
    int32_t acc;

    for(o=0;o<D2_OUT;o++)
    {
        acc=dense2_b[o];

        int base=o*D1_OUT;

        for(i=0;i<D1_OUT;i++)
            acc+=d1_out[i]*dense2_w[base+i];

        d2_out[o]=sat_i8_relu_shift(acc,DENSE2_MULT, DENSE2_ZP, DENSE2_SHIFT);
    }
}

static void dense3_layer(void)
{
    int o,i;
    int32_t acc;

    for(o=0;o<D3_OUT;o++)
    {
        acc=dense3_b[o];

        int base=o*D2_OUT;

        for(i=0;i<D2_OUT;i++)
            acc+=d2_out[i]*dense3_w[base+i];
        acc = (int32_t)(((int64_t)acc * DENSE3_MULT) >> 31);

        if(DENSE3_SHIFT > 0)
            acc = (acc + (1 << (DENSE3_SHIFT - 1))) >> DENSE3_SHIFT;

        acc += DENSE3_ZP;                                        // add zero point

        // Clamp to int8 range
        if (acc > 127) acc = 127;
        if (acc < -128) acc = -128;


        logits[o]=acc;
    }
}

static int argmax_logits(void)
{
    int i,best=0;
    int32_t bestv=logits[0];

    for(i=1;i<D3_OUT;i++)
        if(logits[i]>bestv)
        {
            bestv=logits[i];
            best=i;
        }

    return best;
}

static void print_logits(void)
{
    int i;

    for(i=0;i<D3_OUT;i++)
        printf("[%2d] %-8s : %ld\r\n",i,labels[i],(long)logits[i]);
}

int main(void)
{
    init_platform();

    xil_printf("Running FULL CNN in SOFTWARE\r\n");


    int8_t input_up[49][10] = {
        { -44, 0, -1, -1, -1, -1, -1, 0, 0, 0 },
        { -46, -2, -1, 0, -1, -1, 0, -1, 0, -1 },
        { -45, -1, -1, 1, -1, -1, 0, -1, 0, -1 },
        { -45, -1, 0, 1, -1, 0, 0, -1, 1, -1 },
        { -44, -1, 0, 1, -2, 0, 1, -1, 1, 1 },
        { -44, 0, -2, 1, -2, -1, 1, 0, 0, -1 },
        { -44, 0, -1, 0, -1, 0, 1, 0, -1, -1 },
        { -44, 0, -1, 0, 0, 0, 1, -1, -1, 0 },
        { -17, 2, -1, -6, 2, 1, -2, 0, -1, 0 },
        { 12, 4, -1, -7, 3, 1, -2, 1, -1, -1 },
        { 18, 2, -2, -5, 2, 1, -2, 1, -1, -1 },
        { 22, 1, -2, -4, 0, 1, -1, 1, -1, -1 },
        { 20, 2, -1, -4, -1, 2, -1, 1, 0, 0 },
        { 13, 5, 0, -3, -2, 1, -3, 0, 0, 0 },
        { 4, 5, -1, -2, 0, 2, -1, 0, 0, -1 },
        { -5, 3, -1, -2, 0, 1, -2, 1, 0, -1 },
        { -9, 3, -1, -3, 0, 2, -3, 0, 0, 0 },
        { -13, 4, -2, -2, 0, 1, -2, 0, 0, 0 },
        { -19, 3, -3, -2, -1, 1, -2, -1, 1, -1 },
        { -25, 2, -2, -1, -1, 1, -1, -1, 1, -1 },
        { -22, 0, 0, 0, 0, -1, -1, 1, -1, 1 },
        { -6, -1, -1, 0, -1, -4, -2, -1, -2, 1 },
        { -4, -1, -1, -3, -2, -3, -1, 0, -2, 1 },
        { -12, 0, -2, -2, -1, -3, -1, 1, 0, 0 },
        { -18, 2, -2, -3, -2, -2, 0, 0, 0, 1 },
        { -22, 1, -2, -3, -1, -2, 1, 0, -1, -1 },
        { -25, 2, -1, -2, 0, -1, 0, -1, 0, -1 },
        { -28, 2, -1, 0, -1, -2, 0, -1, 0, -1 },
        { -31, 0, -1, 0, -1, -1, 0, -1, 0, 0 },
        { -30, 2, 0, 0, 0, -2, 0, -1, -1, 0 },
        { -32, 1, 0, 1, 0, -1, 1, 0, -1, 0 },
        { -33, 0, 0, 1, 0, 0, 0, -2, -1, -1 },
        { -33, 0, 0, 1, 0, -1, 0, -2, 0, -1 },
        { -33, 1, -1, 1, 0, -1, -1, -1, 0, 0 },
        { -36, 0, 0, 2, -1, -1, -1, -1, 0, 0 },
        { -38, -1, 1, 2, -1, -1, 0, -2, -1, -1 },
        { -38, 0, 1, 0, -2, -1, 0, -2, -1, 1 },
        { -41, 0, 0, -1, -2, 0, -1, -1, 0, 0 },
        { -41, -1, -1, 0, -1, -1, 0, -1, -1, -1 },
        { -41, 0, -1, 0, -1, -1, 0, -1, -1, -1 },
        { -41, 0, 0, 0, 0, -1, 0, -1, -1, -1 },
        { -43, 0, 0, 1, -2, 0, 1, 0, 0, -1 },
        { -42, 0, 1, 1, -1, 0, 1, -1, 0, -1 },
        { -43, 0, 1, 0, 0, 0, 1, 0, -1, -1 },
        { -43, 0, 0, 0, -1, 0, 0, -1, 0, -1 },
        { -43, 0, -1, -1, -2, -1, 0, -1, -1, 0 },
        { -43, 0, 0, 1, -1, -1, 0, -1, 0, 0 },
        { -44, -1, 0, 1, 0, -1, 0, 0, 1, 0 },
        { -42, -1, -1, 1, 0, -1, 0, -1, -1, -1 },
    };
    //load_demo_input();
    int8_t input_intermediate [49][10];

    quantize_input(input_stop2, input_intermediate);
    memcpy(input_x, input_intermediate, sizeof(input_x));

    // ---------- Measure latency ----------
    XTime tStart, tEnd;
    XTime_GetTime(&tStart);


    conv1_layer_sw();
    conv2_layer_sw();

    maxpool_2x4_s2x4();

    flatten_pooled();

    dense1_layer();
    dense2_layer();
    dense3_layer();

    XTime_GetTime(&tEnd);
    double latency_ms = (double)(tEnd - tStart) * 1000.0 / COUNTS_PER_SECOND;

    int pred=argmax_logits();

    printf("\r\nPrediction index = %d\r\n",pred);
    printf("Predicted label = %s\r\n",labels[pred]);

//    print_logits();

    printf("Full CNN processing time: %.3f ms\r\n", latency_ms);

    cleanup_platform();
    return 0;
}
