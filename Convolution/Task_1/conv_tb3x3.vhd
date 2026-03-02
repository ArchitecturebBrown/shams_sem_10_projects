library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity tb_conv3x3 is
end tb_conv3x3;

architecture behavior of tb_conv3x3 is

    -- Component declaration
    component Full_conv_seq
        port (
            clk           : in  std_logic;
            reset_n       : in  std_logic;
            load_index    : in  unsigned(3 downto 0);
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
    signal load_index  : unsigned(3 downto 0) := (others => '0');
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
        type int_array9 is array(0 to 8) of integer;

        constant cells_array   : int_array9 := (0, 10, 10, 10, 10, 0, 0, 10, 10);
        constant weights_array : int_array9 := (1, 0, -1, 1, 0, -1, 1, 0, -1);

        constant expected_result : integer := -10;

    begin

        reset_n <= '0';
        wait for 20 ns;
        reset_n <= '1';
        wait for 20 ns;

        -- Load activations and weights
        for i in 0 to 8 loop
            load_index  <= to_unsigned(i, 4);
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

        wait;

    end process;

end behavior;
