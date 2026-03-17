
/////////////////////////////////////conv hw led
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xtime_l.h"   // For timing

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
#define IN_H    49
#define IN_W    10
#define IN_C    1

#define C1_KH   10
#define C1_KW   4
#define C1_OC   28
#define C1_SH   1
#define C1_SW   1
#define C1_OH   (((IN_H - C1_KH)/C1_SH) + 1)
#define C1_OW   (((IN_W - C1_KW)/C1_SW) + 1)

#define C2_KH   10
#define C2_KW   4
#define C2_IC   C1_OC
#define C2_OC   30
#define C2_SH   2
#define C2_SW   1
#define C2_OH   (((C1_OH - C2_KH)/C2_SH) + 1)
#define C2_OW   (((C1_OW - C2_KW)/C2_SW) + 1)

#define P_KH    2
#define P_KW    4
#define P_SH    2
#define P_SW    4
#define P_OH    (((C2_OH - P_KH)/P_SH) + 1)
#define P_OW    (((C2_OW - P_KW)/P_SW) + 1)

#define FLAT_LEN   (P_OH * P_OW * C2_OC)
#define D1_OUT     16
#define D2_OUT     128
#define D3_OUT 12

#define NUM_CLASSES D3_OUT

#define CONV1_SHIFT   7
#define CONV2_SHIFT   8
#define DENSE1_SHIFT  7

#define LEN(arr) ((int)(sizeof(arr)/sizeof((arr)[0])))

/* ---------------------- Buffers ---------------------- */
static int8_t  input_x[IN_H][IN_W];
static int8_t  act1[C1_OH][C1_OW][C1_OC];
static int8_t  act2[C2_OH][C2_OW][C2_OC];
static int8_t  actp[P_OH][P_OW][C2_OC];
static int8_t  flat[FLAT_LEN];
static int8_t  d1_out[D1_OUT];
static int8_t d2_out[D2_OUT];
static int32_t logits[D3_OUT];
static int32_t dense1_b_fixed[D1_OUT];

/* ---------------------- GPIOs for hardware conv ---------------------- */
XGpio Gpio0, Gpio1, Gpio2, Gpio3,LED;

/* ---------------------- Labels ---------------------- */
static const char *labels[NUM_CLASSES] = {
   "silence", "unknown", "yes", "no", "up", "down",
   "left", "right", "on", "off", "stop", "go"
};

/* ---------------------- Helpers ---------------------- */
static inline int8_t sat_i8_relu_shift(int32_t x, int shift)
{
   if(x<0) x=0;
   if(shift>0) x>>=shift;
   if(x>127) x=127;
   return (int8_t)x;
}

static void fix_dense1_bias(void)
{
   int i;
   for(i=0;i<D1_OUT;i++) dense1_b_fixed[i]=0;
   int n = LEN(dense1_b);
   if(n>D1_OUT) n=D1_OUT;
   for(i=0;i<n;i++) dense1_b_fixed[i]=dense1_b[i];
}

static void load_demo_input(void)
{
   int h,w;
   for(h=0;h<IN_H;h++)
       for(w=0;w<IN_W;w++)
           input_x[h][w] = (int8_t)(((h*3+w*7)%40)-20);
}

/* ---------------------- Hardware Conv1 ---------------------- */
static void conv1_layer_hw(void)
{
   int oh, ow, oc, kh, kw, index;
   xil_printf("Running Conv1 on HW...\r\n");

   for(oh=0; oh<C1_OH; oh++){
       for(ow=0; ow<C1_OW; ow++){
           for(oc=0; oc<C1_OC; oc++){
               index=0;
               for(kh=0; kh<C1_KH; kh++){
                   for(kw=0; kw<C1_KW; kw++){
                       XGpio_DiscreteWrite(&Gpio0,1,(uint32_t)input_x[oh+kh][ow+kw]);
                       XGpio_DiscreteWrite(&Gpio1,1,(uint32_t)conv1_w[index]);
                       XGpio_DiscreteWrite(&Gpio2,1,index);

                       XGpio_DiscreteWrite(&Gpio0,2,1);
                       XGpio_DiscreteWrite(&Gpio0,2,0);
                       index++;
                   }
               }
               XGpio_DiscreteWrite(&Gpio1,2,1);
               XGpio_DiscreteWrite(&Gpio1,2,0);

               while((XGpio_DiscreteRead(&Gpio3,2)&0x1)==0);

               uint32_t raw = XGpio_DiscreteRead(&Gpio3,1);
               int32_t result = raw & 0xFFFFF;
               if(result & (1<<19)) result |= 0xFFF00000;

               act1[oh][ow][oc] = sat_i8_relu_shift(result, CONV1_SHIFT);

               XGpio_DiscreteWrite(&Gpio2,2,1);
               XGpio_DiscreteWrite(&Gpio2,2,0);
           }
       }
   }
}

/* ---------------------- Hardware Conv2 ---------------------- */
static void conv2_layer_hw(void)
{
   int oh, ow, oc, kh, kw, ic, index;
   xil_printf("Running Conv2 on HW...\r\n");

   for(oh=0; oh<C2_OH; oh++){
       for(ow=0; ow<C2_OW; ow++){
           for(oc=0; oc<C2_OC; oc++){
               index=0;
               for(kh=0; kh<C2_KH; kh++){
                   for(kw=0; kw<C2_KW; kw++){
                       for(ic=0; ic<C2_IC; ic++){
                           XGpio_DiscreteWrite(&Gpio0,1,(uint32_t)act1[oh* C2_SH + kh][ow*C2_SW + kw]);
                           XGpio_DiscreteWrite(&Gpio1,1,(uint32_t)conv2_w[index]);
                           XGpio_DiscreteWrite(&Gpio2,1,index);

                           XGpio_DiscreteWrite(&Gpio0,2,1);
                           XGpio_DiscreteWrite(&Gpio0,2,0);
                           index++;
                       }
                   }
               }
               XGpio_DiscreteWrite(&Gpio1,2,1);
               XGpio_DiscreteWrite(&Gpio1,2,0);

               while((XGpio_DiscreteRead(&Gpio3,2)&0x1)==0);

               uint32_t raw = XGpio_DiscreteRead(&Gpio3,1);
               int32_t result = raw & 0xFFFFF;
               if(result & (1<<19)) result |= 0xFFF00000;

               act2[oh][ow][oc] = sat_i8_relu_shift(result, CONV2_SHIFT);

               XGpio_DiscreteWrite(&Gpio2,2,1);
               XGpio_DiscreteWrite(&Gpio2,2,0);
           }
       }
   }
}

/* ---------------------- Software remaining layers ---------------------- */
static void maxpool_2x4_s2x4(void)
{
   int oh, ow, c, kh, kw;
   for(oh=0; oh<P_OH; oh++)
       for(ow=0; ow<P_OW; ow++)
           for(c=0; c<C2_OC; c++){
               int maxv=-128;
               for(kh=0; kh<P_KH; kh++)
                   for(kw=0; kw<P_KW; kw++){
                       int ih = oh*P_SH+kh;
                       int iw = ow*P_SW+kw;
                       if(act2[ih][iw][c]>maxv) maxv=act2[ih][iw][c];
                   }
               actp[oh][ow][c]=(int8_t)maxv;
           }
}

static void flatten_pooled(void)
{
   int idx=0,h,w,c;
   for(h=0;h<P_OH;h++)
       for(w=0;w<P_OW;w++)
           for(c=0;c<C2_OC;c++)
               flat[idx++]=actp[h][w][c];
}

static void dense1(void) {
   xil_printf("Running Dense1...\r\n");
   for(int o=0; o<D1_OUT; o++) {
       int32_t acc = dense1_b[o];
       int base = o*FLAT_LEN;
       for(int i=0; i<FLAT_LEN; i++)
           acc += flat[i]*dense1_w[base+i];
       d1_out[o] = sat_i8_relu_shift(acc, DENSE1_SHIFT);
   }
}

static void dense2(void) {
   xil_printf("Running Dense2...\r\n");
   for(int o=0; o<D2_OUT; o++) {
       int32_t acc = dense2_b[o];
       int base = o*D1_OUT;
       for(int i=0; i<D1_OUT; i++)
           acc += d1_out[i]*dense2_w[base+i];
       d2_out[o] = sat_i8_relu_shift(acc, DENSE1_SHIFT);
   }
}

static void dense3(void) {
   xil_printf("Running Dense3...\r\n");
   for(int o=0; o<D3_OUT; o++) {
       int32_t acc = dense3_b[o];
       int base = o*D2_OUT;
       for(int i=0; i<D2_OUT; i++)
           acc += d2_out[i]*dense3_w[base+i];
       logits[o] = acc;
   }
}

static int argmax_logits(void)
{
   int i,best_i=0;
   int32_t best_v=logits[0];
   for(i=1;i<D3_OUT;i++)
       if(logits[i]>best_v){best_v=logits[i];best_i=i;}
   return best_i;
}

static void print_logits(void)
{
   for(int i=0;i<D3_OUT;i++)
       printf("[%2d] %-8s : %ld\r\n", i, labels[i], (long)logits[i]);
}

/* ---------------------- Main ---------------------- */
int main(void)
{
   int status;
   XTime tStart, tEnd;
   u64 cycles;
   double time_ms;
       int8_t input_go[49][10] = {
           { -121, 0, -2, -2, -3, -2, -1, 0, 1, 0 },
           { -127, -4, -2, 0, -2, -3, 0, -2, -1, -2 },
           { -124, -3, -1, 1, -2, -1, -1, -2, 0, -2 },
           { -123, -2, 0, 3, -1, 0, 0, -2, 1, -2 },
           { -121, -2, 0, 1, -4, 0, 3, -2, 2, 1 },
           { -122, -1, -4, 1, -5, -3, 4, 0, -1, -1 },
           { -120, 0, -2, 0, -2, -1, 3, -1, -2, -1 },
           { -120, 1, -1, 0, 0, 0, 1, -2, -1, 0 },
           { -46, 6, -4, -15, 4, 2, -5, 0, -1, -1 },
           { 32, 10, -3, -19, 7, 2, -6, 1, -2, -3 },
           { 50, 4, -5, -14, 5, 1, -6, 3, -3, -3 },
           { 59, 2, -4, -9, 0, 3, -3, 1, -2, -2 },
           { 55, 4, -2, -9, -3, 4, -3, 1, 0, -1 },
           { 37, 13, -1, -8, -4, 1, -6, 0, 0, 0 },
           { 9, 14, -1, -5, 0, 5, -3, 0, 0, -2 },
           { -12, 8, -1, -6, 0, 4, -6, 1, 0, -2 },
           { -24, 9, -2, -7, 0, 5, -7, 0, 0, -1 },
           { -35, 10, -6, -6, 0, 3, -4, 0, 0, 0 },
           { -51, 9, -7, -6, -1, 3, -5, -2, 1, -3 },
           { -69, 5, -5, -4, -2, 2, -2, -2, 2, -2 },
           { -60, 0, 0, 0, 0, -3, -1, 2, -3, 2 },
           { -15, -2, -1, 0, -3, -11, -4, -2, -6, 2 },
           { -9, -3, -3, -7, -5, -8, -1, 0, -5, 2 },
           { -33, 0, -5, -6, -1, -9, -1, 2, 0, 0 },
           { -49, 4, -6, -7, -4, -6, -1, 0, 1, 1 },
           { -60, 3, -5, -7, -4, -5, 1, 0, -1, -1 },
           { -68, 5, -3, -4, 0, -3, 0, -2, 0, -1 },
           { -76, 4, -1, 0, -1, -4, 1, -3, 0, -2 },
           { -85, 0, -1, -1, -1, -2, 0, -1, 0, -1 },
           { -83, 5, -1, 0, 0, -4, 0, -2, -2, 0 },
           { -88, 1, 0, 3, 1, -2, 3, 0, -1, -1 },
           { -92, 0, 0, 1, 0, 0, 1, -4, -2, -3 },
           { -92, 0, 0, 2, 0, -1, -1, -5, 0, -1 },
           { -92, 1, -1, 2, 0, -3, -1, -3, 0, 0 },
           { -100, 0, 0, 4, -2, -2, -1, -1, 0, 0 },
           { -104, -2, 1, 4, -2, -2, 0, -4, -1, -2 },
           { -106, 0, 1, 0, -4, -1, 0, -5, -2, 1 },
           { -112, 0, 0, -2, -4, 0, -2, -2, 0, 0 },
           { -114, -2, -2, 0, -3, -2, 0, -2, -2, -1 },
           { -113, 0, -1, 0, -2, -3, 0, -2, -2, -3 },
           { -113, -1, 0, 1, -1, -1, 0, -2, -1, -2 },
           { -119, -1, 0, 2, -5, -1, 1, 0, 0, -1 },
           { -117, 0, 2, 1, -2, 0, 3, -2, 0, -1 },
           { -118, 1, 1, 1, -1, 0, 1, 0, -2, -3 },
           { -118, 0, 0, 0, -3, 0, 1, -1, -1, -1 },
           { -118, 0, -2, -1, -5, -3, 0, -2, -1, -1 },
           { -118, -1, 0, 1, -2, -3, 0, -1, 1, -1 },
           { -122, -2, -1, 3, 0, -2, 0, -1, 2, 0 },
           { -116, -1, -1, 2, -1, -2, -1, -2, -1, -1 },
       };

   init_platform();


   /* Initialize GPIOs for HW convs */
   status = XGpio_Initialize(&Gpio0,XPAR_AXI_GPIO_0_DEVICE_ID);
   status |= XGpio_Initialize(&Gpio1,XPAR_AXI_GPIO_1_DEVICE_ID);
   status |= XGpio_Initialize(&Gpio2,XPAR_AXI_GPIO_2_DEVICE_ID);
   status |= XGpio_Initialize(&Gpio3,XPAR_AXI_GPIO_3_DEVICE_ID);
   status |= XGpio_Initialize(&LED,XPAR_AXI_GPIO_4_DEVICE_ID);
   if(status!=XST_SUCCESS){ xil_printf("GPIO init failed!\r\n"); cleanup_platform(); return -1; }

   /* Set directions */
   XGpio_SetDataDirection(&Gpio0,1,0x0); XGpio_SetDataDirection(&Gpio0,2,0x0);
   XGpio_SetDataDirection(&Gpio1,1,0x0); XGpio_SetDataDirection(&Gpio1,2,0x0);
   XGpio_SetDataDirection(&Gpio2,1,0x0); XGpio_SetDataDirection(&Gpio2,2,0x0);
   XGpio_SetDataDirection(&Gpio3,1,0xFFFFFFFF);
   XGpio_SetDataDirection(&Gpio3,2,0xFFFFFFFF);
   XGpio_SetDataDirection(&LED,1,0xFFFFFFFF);

   memcpy(input_x, input_go, sizeof(input_x));

   /* ===== START TIMING ===== */
   XTime_GetTime(&tStart);

   conv1_layer_hw();
   conv2_layer_hw();
   maxpool_2x4_s2x4();
   flatten_pooled();
   dense1();
   dense2();
   dense3();

   XTime_GetTime(&tEnd);
   /* ===== END TIMING ===== */

   cycles = (u64)(tEnd - tStart);
   time_ms = (double)cycles * 1000.0 / COUNTS_PER_SECOND;

   int pred = argmax_logits();
   printf("\r\nPrediction index = %d\r\n", pred);
   printf("Predicted label  = %s\r\n", labels[pred]);

   if(strcmp(labels[pred], "go") == 0) {
       xil_printf("Turning LED on\n");
       XGpio_DiscreteWrite(&LED, 1, 1);
       sleep(5);
       xil_printf("Turning LED off\n");
       XGpio_DiscreteWrite(&LED, 1, 0);
       sleep(5);
   }

//    print_logits();
//    printf("CPU cycles       = %llu\r\n", cycles);
   printf("Processing Time  = %.6f ms\r\n", time_ms);

   cleanup_platform();
   return 0;
}
