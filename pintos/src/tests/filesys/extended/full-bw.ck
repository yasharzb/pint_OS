# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(full-bw) begin
(full-bw) block_read operation didn't happen
(full-bw) block_write operation invokation count matches the exception
(full-bw) end
full-bw: exit(0)
EOF
pass;