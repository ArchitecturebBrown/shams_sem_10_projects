
///////////////////////TASK 1 WITH FULL FEATURE MAP OUTPUT 3X3

#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpio.h"

// GPIO handles
XGpio Gpio0, Gpio1, Gpio2, Gpio3;

// Sign-extend 20-bit result (if needed)
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
   // Load cells and weights
   // ------------------------
   int8_t cells[9]   = {0,10,10,10,10,0,0,10,10};
   int8_t weights[9] = {1,0,-1,1,0,-1,1,0,-1};

   for(int i=0; i<9; i++){
       XGpio_DiscreteWrite(&Gpio0, 1, (uint32_t)cells[i]);    // activation
       XGpio_DiscreteWrite(&Gpio2, 1, i);                     // load_index
       XGpio_DiscreteWrite(&Gpio1, 1, (uint32_t)weights[i]);  // weights
       XGpio_DiscreteWrite(&Gpio0, 2, 1); // pulse load_enable
       XGpio_DiscreteWrite(&Gpio0, 2, 0);
   }

   // ------------------------
   // Start convolution
   // ------------------------
   XGpio_DiscreteWrite(&Gpio1, 2, 1); // start pulse
   XGpio_DiscreteWrite(&Gpio1, 2, 0);

   // ------------------------
   // Wait for done
   // ------------------------
   while((XGpio_DiscreteRead(&Gpio3, 2) & 0x1) == 0);

   // ------------------------
   // Read and print result
   // ------------------------
   uint32_t raw = XGpio_DiscreteRead(&Gpio3, 1);
   int32_t val = sign_extend_20bit(raw);
   xil_printf("Convolution result: %d\n\r", val);

   // ------------------------
   // Clear done
   // ------------------------
   XGpio_DiscreteWrite(&Gpio2, 2, 1);
   XGpio_DiscreteWrite(&Gpio2, 2, 0);

   cleanup_platform();
   return 0;
}





