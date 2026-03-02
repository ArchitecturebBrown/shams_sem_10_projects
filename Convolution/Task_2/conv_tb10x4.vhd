
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity tb_conv10x4 is
end tb_conv10x4;

architecture behavior of tb_conv10x4 is

   -- Component declaration
   component Full_conv_seq
       port (
           clk           : in  std_logic;
           reset_n       : in  std_logic;
           load_index    : in  unsigned(5 downto 0); -- 0 to 39
           load_enable   : in  std_logic;
           activation    : in  signed(7 downto 0);
           weights       : in  signed(7 downto 0);
           start         : in  std_logic;
           clear_done    : in  std_logic;
           done          : out std_logic;
           result        : out signed(31 downto 0)
       );
   end component;

   -- Signals
   signal clk         : std_logic := '0';
   signal reset_n     : std_logic := '0';
   signal load_index  : unsigned(5 downto 0) := (others => '0');
   signal load_enable : std_logic := '0';
   signal activation  : signed(7 downto 0) := (others => '0');
   signal weights     : signed(7 downto 0) := (others => '0');
   signal start       : std_logic := '0';
   signal clear_done  : std_logic := '0';
   signal done        : std_logic;
   signal result      : signed(31 downto 0);

   constant clk_period : time := 10 ns;

begin

   -- Instantiate DUT
   uut: Full_conv_seq
       port map (
           clk         => clk,
           reset_n     => reset_n,
           load_index  => load_index,
           load_enable => load_enable,
           activation  => activation,
           weights     => weights,
           start       => start,
           clear_done  => clear_done,
           done        => done,
           result      => result
       );

   -- Clock generation
   clk_process : process
   begin
       while true loop
           clk <= '0';
           wait for clk_period/2;
           clk <= '1';
           wait for clk_period/2;
       end loop;
   end process;

   -- Stimulus process
   stim_proc: process
       -- Array type for 40 elements
       type int_array40 is array(0 to 39) of integer;

       -- Example activations and weights
       constant cells_array   : int_array40 := (
           0, 10, 10, 10, 10, 0, 0, 10, 10, 5,
           1, 2, 3, 4, 5, 6, 7, 8, 9, 0,
           1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
           3, 3, 3, 3, 3, 4, 4, 4, 4, 4
       );

       constant weights_array : int_array40 := (
           1, 0, -1, 1, 0, -1, 1, 0, -1, 1,
           1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
           1, -1, 1, -1, 1, 0, -1, 0, 1, -1,
           1, 1, 1, -1, -1, 0, 0, 1, -1, 0
       );

       -- Expected result (precomputed)
       constant expected_result : integer := 95;  -- replace with correct sum if needed

   begin

       -- Apply reset
       reset_n <= '0';
       wait for 20 ns;
       reset_n <= '1';
       wait for 20 ns;

       -- Load 40 activations and weights
       for i in 0 to 39 loop
           load_index  <= to_unsigned(i, 6);
           activation  <= to_signed(cells_array(i), 8);
           weights     <= to_signed(weights_array(i), 8);
           load_enable <= '1';
           wait for clk_period;
       end loop;

       load_enable <= '0';
       wait for 20 ns;

       -- Start computation
       start <= '1';
       wait for clk_period;
       start <= '0';

       -- Wait until done
       wait until done = '1';
       wait for clk_period;

       -- Self-check
       assert result = to_signed(expected_result, 32)
       report "Convolution output mismatch!"
       severity error;

       -- Clear done
       clear_done <= '1';
       wait for clk_period;
       clear_done <= '0';

       wait for 50 ns;

       -- Stop simulation
       wait;

   end process;

end behavior;
