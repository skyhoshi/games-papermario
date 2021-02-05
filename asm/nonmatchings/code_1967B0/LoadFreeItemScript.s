.set noat      # allow manual use of $at
.set noreorder # don't insert nops after branches

glabel LoadFreeItemScript
/* 1968FC 8026801C 27BDFFD8 */  addiu     $sp, $sp, -0x28
/* 196900 80268020 AFB3001C */  sw        $s3, 0x1c($sp)
/* 196904 80268024 0080982D */  daddu     $s3, $a0, $zero
/* 196908 80268028 AFB10014 */  sw        $s1, 0x14($sp)
/* 19690C 8026802C 3C11800E */  lui       $s1, %hi(gBattleStatus)
/* 196910 80268030 2631C070 */  addiu     $s1, $s1, %lo(gBattleStatus)
/* 196914 80268034 3C038008 */  lui       $v1, %hi(gItemTable)
/* 196918 80268038 246378E0 */  addiu     $v1, $v1, %lo(gItemTable)
/* 19691C 8026803C AFBF0020 */  sw        $ra, 0x20($sp)
/* 196920 80268040 AFB20018 */  sw        $s2, 0x18($sp)
/* 196924 80268044 AFB00010 */  sw        $s0, 0x10($sp)
/* 196928 80268048 8622017A */  lh        $v0, 0x17a($s1)
/* 19692C 8026804C 8E3000D8 */  lw        $s0, 0xd8($s1)
/* 196930 80268050 00021140 */  sll       $v0, $v0, 5
/* 196934 80268054 00439021 */  addu      $s2, $v0, $v1
/* 196938 80268058 8E420008 */  lw        $v0, 8($s2)
/* 19693C 8026805C 0200202D */  daddu     $a0, $s0, $zero
/* 196940 80268060 AE200188 */  sw        $zero, 0x188($s1)
/* 196944 80268064 34428000 */  ori       $v0, $v0, 0x8000
/* 196948 80268068 0C098C0B */  jal       player_create_target_list
/* 19694C 8026806C AE220184 */   sw       $v0, 0x184($s1)
/* 196950 80268070 8203040D */  lb        $v1, 0x40d($s0)
/* 196954 80268074 00031080 */  sll       $v0, $v1, 2
/* 196958 80268078 00431021 */  addu      $v0, $v0, $v1
/* 19695C 8026807C 00021080 */  sll       $v0, $v0, 2
/* 196960 80268080 2442022C */  addiu     $v0, $v0, 0x22c
/* 196964 80268084 02028021 */  addu      $s0, $s0, $v0
/* 196968 80268088 96020000 */  lhu       $v0, ($s0)
/* 19696C 8026808C A62201A0 */  sh        $v0, 0x1a0($s1)
/* 196970 80268090 92020003 */  lbu       $v0, 3($s0)
/* 196974 80268094 3C038029 */  lui       $v1, %hi(D_80293B80)
/* 196978 80268098 24633B80 */  addiu     $v1, $v1, %lo(D_80293B80)
/* 19697C 8026809C A22201A2 */  sb        $v0, 0x1a2($s1)
/* 196980 802680A0 8C620000 */  lw        $v0, ($v1)
/* 196984 802680A4 1040000B */  beqz      $v0, .L802680D4
/* 196988 802680A8 0000802D */   daddu    $s0, $zero, $zero
/* 19698C 802680AC 8624017A */  lh        $a0, 0x17a($s1)
/* 196990 802680B0 8C620000 */  lw        $v0, ($v1)
.L802680B4:
/* 196994 802680B4 10440005 */  beq       $v0, $a0, .L802680CC
/* 196998 802680B8 24630004 */   addiu    $v1, $v1, 4
/* 19699C 802680BC 8C620000 */  lw        $v0, ($v1)
/* 1969A0 802680C0 1440FFFC */  bnez      $v0, .L802680B4
/* 1969A4 802680C4 26100001 */   addiu    $s0, $s0, 1
/* 1969A8 802680C8 8C620000 */  lw        $v0, ($v1)
.L802680CC:
/* 1969AC 802680CC 14400004 */  bnez      $v0, .L802680E0
/* 1969B0 802680D0 00000000 */   nop
.L802680D4:
/* 1969B4 802680D4 96420018 */  lhu       $v0, 0x18($s2)
/* 1969B8 802680D8 30420080 */  andi      $v0, $v0, 0x80
/* 1969BC 802680DC 2C500001 */  sltiu     $s0, $v0, 1
.L802680E0:
/* 1969C0 802680E0 3C028029 */  lui       $v0, %hi(gBattleItemTable)
/* 1969C4 802680E4 24423C04 */  addiu     $v0, $v0, %lo(gBattleItemTable)
/* 1969C8 802680E8 00108100 */  sll       $s0, $s0, 4
/* 1969CC 802680EC 02028021 */  addu      $s0, $s0, $v0
/* 1969D0 802680F0 8E040000 */  lw        $a0, ($s0)
/* 1969D4 802680F4 8E050004 */  lw        $a1, 4($s0)
/* 1969D8 802680F8 0C00A5CF */  jal       dma_copy
/* 1969DC 802680FC 8E060008 */   lw       $a2, 8($s0)
/* 1969E0 80268100 8E04000C */  lw        $a0, 0xc($s0)
/* 1969E4 80268104 24030001 */  addiu     $v1, $zero, 1
/* 1969E8 80268108 AE630088 */  sw        $v1, 0x88($s3)
/* 1969EC 8026810C AE640084 */  sw        $a0, 0x84($s3)
/* 1969F0 80268110 8FBF0020 */  lw        $ra, 0x20($sp)
/* 1969F4 80268114 8FB3001C */  lw        $s3, 0x1c($sp)
/* 1969F8 80268118 8FB20018 */  lw        $s2, 0x18($sp)
/* 1969FC 8026811C 8FB10014 */  lw        $s1, 0x14($sp)
/* 196A00 80268120 8FB00010 */  lw        $s0, 0x10($sp)
/* 196A04 80268124 24020002 */  addiu     $v0, $zero, 2
/* 196A08 80268128 03E00008 */  jr        $ra
/* 196A0C 8026812C 27BD0028 */   addiu    $sp, $sp, 0x28
