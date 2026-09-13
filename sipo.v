module sipo (
    input wire clk,
    input wire serial_in,
    input wire clear,
    input wire enable,
    output reg [15:0] parallel_out
);

    always @(posedge clk) begin
        if(clear) begin
            parallel_out <= 16'b0;
        end else if(enable) begin
            parallel_out <= {serial_in, parallel_out[15:1]};
        end
    end
    
endmodule
