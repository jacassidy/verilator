// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

class Node;
  int value;
  Node next;
  function new(int v);
    value = v;
  endfunction
endclass

module t (
    input clk
);
  int cyc = 0;
  Node head;
  int sum;

  always @(posedge clk) begin
    cyc <= cyc + 1;
    head = null;
    for (int i = 0; i <= cyc % 7; i++) begin
      automatic Node n = new(i);
      n.next = head;
      head = n;
    end
    sum = 0;
    for (Node p = head; p != null; p = p.next) sum += p.value;
    if (sum != (cyc % 7) * (cyc % 7 + 1) / 2) $stop;
    if (cyc == 50) begin
      $write("*-* All Finished *-*\n");
      $finish;
    end
  end
endmodule
