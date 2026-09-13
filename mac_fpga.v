module mac_fpga (
    input wire [1:0] command,
    input wire clk,
    input wire data_in,
    output wire data_out,
    output wire [5:0] leds
);

    wire [15:0] a_out;
    wire [15:0] b_out;
    wire [31:0] sum_in;
    wire [31:0] sum_out;
    
    wire clear;
    wire ab_enable;
    wire c_enable;
    wire parallel_load;

    // A SIPO 
    sipo a(
        .clk(clk), // Güvenli tasarım için ham saat sinyali bağlandı
        .serial_in(data_in),
        .clear(clear),
        .enable(ab_enable),
        .parallel_out(a_out)
    );

    // B SIPO
    sipo b(
        .clk(clk), // Güvenli tasarım için ham saat sinyali bağlandı
        .serial_in(a_out[0]),
        .clear(clear),
        .enable(ab_enable),
        .parallel_out(b_out)
    );

    // C PIPO
    pipo c(
        .clk(clk),
        .clear(clear),
        .enable(c_enable),
        .parallel_load(parallel_load),
        .serial_in(sum_in[31]), 
        .parallel_in(sum_out),  
        .parallel_out(sum_in)   
    );
    
    // Kontrol Sinyalleri 
    assign ab_enable     = ~command[1];
    assign c_enable      = command[1] | ~command[0];
    assign parallel_load = command[1] & (~command[0]);
    assign clear         = ~(command[1] | command[0]); // Reset yerine clear bağlandı

    // MAC
    assign sum_out  = (a_out * b_out) + sum_in;
    assign data_out = sum_in[31];
   
    assign leds[5:4] = a_out[1:0];      // A register'ının son 2 biti
    assign leds[3:2] = b_out[1:0];      // B register'ının son 2 biti
    assign leds[1:0] = sum_in[1:0];     // Akümülatörün son 2 biti

endmodule
