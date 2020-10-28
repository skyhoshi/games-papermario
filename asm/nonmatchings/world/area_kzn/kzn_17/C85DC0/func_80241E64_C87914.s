.set noat      # allow manual use of $at
.set noreorder # don't insert nops after branches

glabel func_80241E64_C87914
/* C87914 80241E64 27BDFFB8 */  addiu     $sp, $sp, -0x48
/* C87918 80241E68 AFB3003C */  sw        $s3, 0x3c($sp)
/* C8791C 80241E6C 0080982D */  daddu     $s3, $a0, $zero
/* C87920 80241E70 AFBF0040 */  sw        $ra, 0x40($sp)
/* C87924 80241E74 AFB20038 */  sw        $s2, 0x38($sp)
/* C87928 80241E78 AFB10034 */  sw        $s1, 0x34($sp)
/* C8792C 80241E7C AFB00030 */  sw        $s0, 0x30($sp)
/* C87930 80241E80 8E710148 */  lw        $s1, 0x148($s3)
/* C87934 80241E84 0C00EABB */  jal       get_npc_unsafe
/* C87938 80241E88 86240008 */   lh       $a0, 8($s1)
/* C8793C 80241E8C 0040802D */  daddu     $s0, $v0, $zero
/* C87940 80241E90 8E22007C */  lw        $v0, 0x7c($s1)
/* C87944 80241E94 8603008E */  lh        $v1, 0x8e($s0)
/* C87948 80241E98 2442FFFF */  addiu     $v0, $v0, -1
/* C8794C 80241E9C 14620012 */  bne       $v1, $v0, .L80241EE8
/* C87950 80241EA0 00000000 */   nop      
/* C87954 80241EA4 C6000038 */  lwc1      $f0, 0x38($s0)
/* C87958 80241EA8 4600020D */  trunc.w.s $f8, $f0
/* C8795C 80241EAC 44024000 */  mfc1      $v0, $f8
/* C87960 80241EB0 00000000 */  nop       
/* C87964 80241EB4 A6220010 */  sh        $v0, 0x10($s1)
/* C87968 80241EB8 C600003C */  lwc1      $f0, 0x3c($s0)
/* C8796C 80241EBC 4600020D */  trunc.w.s $f8, $f0
/* C87970 80241EC0 44024000 */  mfc1      $v0, $f8
/* C87974 80241EC4 00000000 */  nop       
/* C87978 80241EC8 A6220012 */  sh        $v0, 0x12($s1)
/* C8797C 80241ECC C6000040 */  lwc1      $f0, 0x40($s0)
/* C87980 80241ED0 24020001 */  addiu     $v0, $zero, 1
/* C87984 80241ED4 A2220007 */  sb        $v0, 7($s1)
/* C87988 80241ED8 4600020D */  trunc.w.s $f8, $f0
/* C8798C 80241EDC 44024000 */  mfc1      $v0, $f8
/* C87990 80241EE0 00000000 */  nop       
/* C87994 80241EE4 A6220014 */  sh        $v0, 0x14($s1)
.L80241EE8:
/* C87998 80241EE8 C6000038 */  lwc1      $f0, 0x38($s0)
/* C8799C 80241EEC C602003C */  lwc1      $f2, 0x3c($s0)
/* C879A0 80241EF0 C6040040 */  lwc1      $f4, 0x40($s0)
/* C879A4 80241EF4 C6060018 */  lwc1      $f6, 0x18($s0)
/* C879A8 80241EF8 E7A00020 */  swc1      $f0, 0x20($sp)
/* C879AC 80241EFC E7A20024 */  swc1      $f2, 0x24($sp)
/* C879B0 80241F00 E7A40028 */  swc1      $f4, 0x28($sp)
/* C879B4 80241F04 E7A60010 */  swc1      $f6, 0x10($sp)
/* C879B8 80241F08 C600000C */  lwc1      $f0, 0xc($s0)
/* C879BC 80241F0C E7A00014 */  swc1      $f0, 0x14($sp)
/* C879C0 80241F10 860200A8 */  lh        $v0, 0xa8($s0)
/* C879C4 80241F14 27A50020 */  addiu     $a1, $sp, 0x20
/* C879C8 80241F18 44820000 */  mtc1      $v0, $f0
/* C879CC 80241F1C 00000000 */  nop       
/* C879D0 80241F20 46800020 */  cvt.s.w   $f0, $f0
/* C879D4 80241F24 E7A00018 */  swc1      $f0, 0x18($sp)
/* C879D8 80241F28 860200A6 */  lh        $v0, 0xa6($s0)
/* C879DC 80241F2C 27A60024 */  addiu     $a2, $sp, 0x24
/* C879E0 80241F30 44820000 */  mtc1      $v0, $f0
/* C879E4 80241F34 00000000 */  nop       
/* C879E8 80241F38 46800020 */  cvt.s.w   $f0, $f0
/* C879EC 80241F3C E7A0001C */  swc1      $f0, 0x1c($sp)
/* C879F0 80241F40 8E040080 */  lw        $a0, 0x80($s0)
/* C879F4 80241F44 0C037711 */  jal       func_800DDC44
/* C879F8 80241F48 27A70028 */   addiu    $a3, $sp, 0x28
/* C879FC 80241F4C 0040902D */  daddu     $s2, $v0, $zero
/* C87A00 80241F50 16400005 */  bnez      $s2, .L80241F68
/* C87A04 80241F54 00000000 */   nop      
/* C87A08 80241F58 8E050018 */  lw        $a1, 0x18($s0)
/* C87A0C 80241F5C 8E06000C */  lw        $a2, 0xc($s0)
/* C87A10 80241F60 0C00EA95 */  jal       npc_move_heading
/* C87A14 80241F64 0200202D */   daddu    $a0, $s0, $zero
.L80241F68:
/* C87A18 80241F68 8602008E */  lh        $v0, 0x8e($s0)
/* C87A1C 80241F6C 9603008E */  lhu       $v1, 0x8e($s0)
/* C87A20 80241F70 18400007 */  blez      $v0, .L80241F90
/* C87A24 80241F74 2462FFFF */   addiu    $v0, $v1, -1
/* C87A28 80241F78 A602008E */  sh        $v0, 0x8e($s0)
/* C87A2C 80241F7C 00021400 */  sll       $v0, $v0, 0x10
/* C87A30 80241F80 18400003 */  blez      $v0, .L80241F90
/* C87A34 80241F84 00000000 */   nop      
/* C87A38 80241F88 12400008 */  beqz      $s2, .L80241FAC
/* C87A3C 80241F8C 00000000 */   nop      
.L80241F90:
/* C87A40 80241F90 8E2200CC */  lw        $v0, 0xcc($s1)
/* C87A44 80241F94 A2200007 */  sb        $zero, 7($s1)
/* C87A48 80241F98 8C420028 */  lw        $v0, 0x28($v0)
/* C87A4C 80241F9C A600008E */  sh        $zero, 0x8e($s0)
/* C87A50 80241FA0 AE020028 */  sw        $v0, 0x28($s0)
/* C87A54 80241FA4 2402000F */  addiu     $v0, $zero, 0xf
/* C87A58 80241FA8 AE620070 */  sw        $v0, 0x70($s3)
.L80241FAC:
/* C87A5C 80241FAC 8FBF0040 */  lw        $ra, 0x40($sp)
/* C87A60 80241FB0 8FB3003C */  lw        $s3, 0x3c($sp)
/* C87A64 80241FB4 8FB20038 */  lw        $s2, 0x38($sp)
/* C87A68 80241FB8 8FB10034 */  lw        $s1, 0x34($sp)
/* C87A6C 80241FBC 8FB00030 */  lw        $s0, 0x30($sp)
/* C87A70 80241FC0 03E00008 */  jr        $ra
/* C87A74 80241FC4 27BD0048 */   addiu    $sp, $sp, 0x48