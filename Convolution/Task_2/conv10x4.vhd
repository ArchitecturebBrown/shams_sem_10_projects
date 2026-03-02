---------------------------------------------task 2 10x4 code-------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity Full_conv_seq is
   port (
       clk           : in  std_logic;
       reset_n       : in  std_logic; -- active low

       load_index    : in  unsigned(5 downto 0); -- 0 to 39
       load_enable   : in  std_logic;

       activation    : in  signed(7 downto 0);
       weights       : in  signed(7 downto 0);

       start         : in  std_logic;
       clear_done    : in  std_logic;

       done          : out std_logic;
       result        : out signed(31 downto 0)
   );
end Full_conv_seq;

architecture rtl of Full_conv_seq is

   type array40 is array (0 to 39) of signed(7 downto 0);

   signal act_mem   : array40 := (others => (others => '0'));
   signal w_mem     : array40 := (others => (others => '0'));

   signal acc       : signed(31 downto 0) := (others => '0');
   signal counter   : integer range 0 to 39 := 0;

   type state_type is (IDLE, COMPUTE, DONE_STATE);
   signal state : state_type := IDLE;

   signal done_reg : std_logic := '0';

begin

   done <= done_reg;
   result <= acc;

   process(clk, reset_n)
   begin
       if reset_n = '0' then
           act_mem   <= (others => (others => '0'));
           w_mem     <= (others => (others => '0'));
           acc       <= (others => '0');
           counter   <= 0;
           state     <= IDLE;
           done_reg  <= '0';

       elsif rising_edge(clk) then

           -- Load phase
           if load_enable = '1' then
               act_mem(to_integer(load_index)) <= activation;
               w_mem(to_integer(load_index))   <= weights;
           end if;

           case state is

               when IDLE =>
                   acc     <= (others => '0');
                   counter <= 0;

                   if start = '1' then
                       state <= COMPUTE;
                   end if;

               when COMPUTE =>
                   acc <= acc + resize(act_mem(counter) * w_mem(counter), 32);
                   counter <= counter + 1;

                   if counter = 39 then
                       state <= DONE_STATE;
                   end if;

               when DONE_STATE =>
                   done_reg <= '1';

                   if clear_done = '1' then
                       done_reg <= '0';
                       state <= IDLE;
                   end if;

           end case;
       end if;
   end process;

end rtl;

