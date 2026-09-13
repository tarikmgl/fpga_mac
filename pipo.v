module pipo(
    input wire clk,
    input wire clear,
    input wire parallel_load,
    input wire serial_in,
    input wire enable,
    input wire [31:0] parallel_in,
    output reg [31:0] parallel_out
);
    always @(posedge clk) begin
        if(clear) begin
            parallel_out <= 32'b0;
        end else if(enable) begin
            if(parallel_load) begin
                parallel_out <= parallel_in;
            end else begin
                parallel_out <= {parallel_out[30:0], serial_in};
            end
        end
    end
endmodule
