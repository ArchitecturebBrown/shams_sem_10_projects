/////////////////////TASK 1 WITH FULL FEATURE MAP OUTPUT 10X4

#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpio.h"

// GPIO handles
XGpio Gpio0, Gpio1, Gpio2, Gpio3;

static int32_t sign_extend_20bit(uint32_t raw)
{
   raw &= 0xFFFFF;
   if(raw & (1u << 19)) raw |= 0xFFF00000;
   return (int32_t)raw;
}

int main() {
   int status;
   init_platform();

   // ------------------------
   // Initialize GPIOs
   // ------------------------
   status = XGpio_Initialize(&Gpio0, XPAR_AXI_GPIO_0_DEVICE_ID);
   status |= XGpio_Initialize(&Gpio1, XPAR_AXI_GPIO_1_DEVICE_ID);
   status |= XGpio_Initialize(&Gpio2, XPAR_AXI_GPIO_2_DEVICE_ID);
   status |= XGpio_Initialize(&Gpio3, XPAR_AXI_GPIO_3_DEVICE_ID);

   if(status != XST_SUCCESS){
       xil_printf("GPIO Initialization Failed!\n\r");
       cleanup_platform();
       return -1;
   }
   xil_printf("GPIO Initialization Successful!\n\r");

   // ------------------------
   // Set GPIO Directions
   // ------------------------
   XGpio_SetDataDirection(&Gpio0, 1, 0x0); // activation OUT
   XGpio_SetDataDirection(&Gpio0, 2, 0x0); // load_enable OUT
   XGpio_SetDataDirection(&Gpio1, 1, 0x0); // weights OUT
   XGpio_SetDataDirection(&Gpio1, 2, 0x0); // start OUT
   XGpio_SetDataDirection(&Gpio2, 1, 0x0); // load_index OUT
   XGpio_SetDataDirection(&Gpio2, 2, 0x0); // clear_done OUT
   XGpio_SetDataDirection(&Gpio3, 1, 0xFFFFFFFF); // result IN
   XGpio_SetDataDirection(&Gpio3, 2, 0xFFFFFFFF); // done IN

   // ------------------------
   // 12x6 Input Activation
   // ------------------------
   int8_t input[12][6] = {
       {0,10,10,10,10,0},
       {0,10,10,10,10,0},
       {0,10,10,10,10,0},
       {0,10,10,10,10,0},
       {0,10,10,10,10,0},
       {0,10,10,10,10,0},
       {0,10,10,10,10,0},
       {0,10,10,10,10,0},
       {0,10,10,10,10,0},
       {0,10,10,10,10,0},
       {0,10,10,10,10,0},
       {0,10,10,10,10,0}
   };

   // ------------------------
   // 10x4 Filter
   // ------------------------
   int8_t filter[10][4] = {
       {1,0,-1,1},
       {1,0,-1,1},
       {1,0,-1,1},
       {1,0,-1,1},
       {1,0,-1,1},
       {1,0,-1,1},
       {1,0,-1,1},
       {1,0,-1,1},
       {1,0,-1,1},
       {1,0,-1,1}
   };

   // ------------------------
   // Sliding window convolution
   // ------------------------
   int out_h = 12 - 10 + 1; // 3
   int out_w = 6 - 4 + 1;   // 3

   xil_printf("Starting full feature map computation (10x4 filter)\n\r");

   for(int row = 0; row < out_h; row++){
       for(int col = 0; col < out_w; col++){
           int index = 0;

           for(int i = 0; i < 10; i++){     // filter height
               for(int j = 0; j < 4; j++){  // filter width
                   XGpio_DiscreteWrite(&Gpio0, 1, (uint32_t)input[row+i][col+j]); // activation
                   XGpio_DiscreteWrite(&Gpio1, 1, (uint32_t)filter[i][j]);        // weight
                   XGpio_DiscreteWrite(&Gpio2, 1, index);                         // load_index

                   // Pulse load_enable
                   XGpio_DiscreteWrite(&Gpio0, 2, 1);
                   XGpio_DiscreteWrite(&Gpio0, 2, 0);

                   index++;
               }
           }

           // Start convolution
           XGpio_DiscreteWrite(&Gpio1, 2, 1);
           XGpio_DiscreteWrite(&Gpio1, 2, 0);

           // Wait done
           while((XGpio_DiscreteRead(&Gpio3, 2) & 0x1) == 0);

           // Read result
           uint32_t raw = XGpio_DiscreteRead(&Gpio3, 1);
           int32_t result = sign_extend_20bit(raw);
           xil_printf("%6d ", result);

           // Clear done
           XGpio_DiscreteWrite(&Gpio2, 2, 1);
           XGpio_DiscreteWrite(&Gpio2, 2, 0);
       }
       xil_printf("\n\r");
   }

   xil_printf("Feature map complete\n\r");

   cleanup_platform();
   return 0;
}


