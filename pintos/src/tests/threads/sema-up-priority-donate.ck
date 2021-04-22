# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(sema-up-priority-donate) A
(sema-up-priority-donate) B
(sema-up-priority-donate) C
(sema-up-priority-donate) D
(sema-up-priority-donate) E
EOF
pass;
