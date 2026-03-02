library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.my_type.all;

entity tb_Buffer1 is
end tb_Buffer1;

architecture behavior of tb_Buffer1 is

    -----------------------------------------------------------------
    -- Component declaration (must match your corrected buffer.vhd)
    -----------------------------------------------------------------
    component Buffer1
        generic (
            FILTER_SIZE  : integer := 3;
            FILTER_total : integer := 9;
            CELL_ROWS    : integer := 6;
            CELL_COLS    : integer := 6;
            cells_total  : integer := 36;
            pixels_total : integer := 16
        );
        port(
            CLK         : in  std_logic;
            reset       : in  std_logic;  -- active LOW

            filter_in   : in  signed(7 downto 0);
            load_filter : in  std_logic;

            cell_in     : in  signed(7 downto 0);
            load_cell   : in  std_logic;

            pixel_out   : out signed(19 downto 0);
            pixel_valid : out std_logic;

            f_d      : out std_logic;
            C_d      : out std_logic
        );
    end component;

    -----------------------------------------------------------------
    -- Signals
    -----------------------------------------------------------------
    signal clk         : std_logic := '0';
    signal reset       : std_logic := '0';

    signal filter_in   : signed(7 downto 0) := (others => '0');
    signal load_filter : std_logic := '0';

    signal cell_in     : signed(7 downto 0) := (others => '0');
    signal load_cell   : std_logic := '0';

    signal pixel_out   : signed(19 downto 0) := (others => '0');
    signal pixel_valid : std_logic := '0';

    signal f_done_sig  : std_logic := '0';
    signal c_done_sig  : std_logic := '0';

    -----------------------------------------------------------------
    -- Test data
    -----------------------------------------------------------------
    type int_array9  is array(0 to 8) of integer;
    type int_array36 is array(0 to 35) of integer;

    constant filter_data : int_array9 := (
         1, 0,-1,
         1, 0,-1,
         1, 0,-1
    );

    constant cell_data : int_array36 := (
        0,10,10,10,10,0,
        0,10,10,10,10,0,
        0,10,10,10,10,0,
        0,10,10,10,10,0,
        0,10,10,10,10,0,
        0,10,10,10,10,0
    );

begin

    -----------------------------------------------------------------
    -- Clock generation: 20 ns period = 50 MHz (matches your hardware now)
    -----------------------------------------------------------------
    clk_process : process
    begin
        while true loop
            clk <= '0';
            wait for 10 ns;
            clk <= '1';
            wait for 10 ns;
        end loop;
    end process;

    -----------------------------------------------------------------
    -- Instantiate DUT
    -----------------------------------------------------------------
    DUT: Buffer1
        generic map(
            FILTER_SIZE  => 3,
            FILTER_total => 9,
            CELL_ROWS    => 6,
            CELL_COLS    => 6,
            cells_total  => 36,
            pixels_total => 16
        )
        port map(
            CLK         => clk,
            reset       => reset,
            filter_in   => filter_in,
            load_filter => load_filter,
            cell_in     => cell_in,
            load_cell   => load_cell,
            pixel_out   => pixel_out,
            pixel_valid => pixel_valid,
            f_d      => f_done_sig,
            C_d      => c_done_sig
        );

    -----------------------------------------------------------------
    -- Stimulus process
    -----------------------------------------------------------------
    stim_proc : process
        variable k : integer;
    begin
        -- Reset (active LOW)
        reset <= '0';
        wait for 40 ns;
        reset <= '1';
        wait for 40 ns;

        -- Load 9 filter values
        for k in 0 to 8 loop
            filter_in   <= to_signed(filter_data(k), 8);
            load_filter <= '1';
            wait for 20 ns;     -- one clock cycle at 50 MHz
            load_filter <= '0';
            wait for 20 ns;     -- gap
        end loop;

        -- Load 36 cell values
        for k in 0 to 35 loop
            cell_in   <= to_signed(cell_data(k), 8);
            load_cell <= '1';
            wait for 20 ns;     -- one clock cycle
            load_cell <= '0';
            wait for 20 ns;     -- gap
        end loop;

        -- Wait until the DUT produces a valid output
        wait until pixel_valid = '1';
        wait for 100 ns;

        report "f_done = " & std_logic'image(f_done_sig);
        report "c_done = " & std_logic'image(c_done_sig);
        report "pixel_valid = " & std_logic'image(pixel_valid);
        report "pixel_out (signed int) = " & integer'image(to_integer(pixel_out));

        -- Stop simulation
        wait;
    end process;

end behavior;