#include "fm2_init.h"

DEFINE_REX_FUNC(sub_829558A0) {
	REX_FUNC_PROLOGUE(sub_829558A0);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a4
	ctx.lr = 0x829558A8;
	__savegprlr_27(ctx, base);
	// stwu r1,-160(r1)
	ea = -160 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r27,r3
	ctx.r27.u64 = ctx.r3.u64;
	// mr r28,r6
	ctx.r28.u64 = ctx.r6.u64;
	// lwz r11,8(r27)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r27.u32 + 8);
	// lwz r9,0(r27)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r27.u32 + 0);
	// lwz r8,4(r27)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r27.u32 + 4);
	// cmplwi cr6,r11,4
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 4, ctx.xer);
	// bge cr6,0x829558ec
	if (!ctx.cr6.lt) goto loc_829558EC;
	// lwz r7,0(r8)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subfic r6,r11,4
	ctx.xer.ca = ctx.r11.u32 <= 4;
	ctx.r6.u64 = static_cast<uint64_t>(4) - ctx.r11.u64;
	// addi r31,r11,28
	ctx.r31.s64 = ctx.r11.s64 + 28;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// slw r10,r7,r11
	ctx.r10.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r7.u32 << (ctx.r11.u8 & 0x3F));
	// srw r30,r7,r6
	ctx.r30.u64 = ctx.r6.u8 & 0x20 ? 0 : (ctx.r7.u32 >> (ctx.r6.u8 & 0x3F));
	// or r11,r10,r9
	ctx.r11.u64 = ctx.r10.u64 | ctx.r9.u64;
	// clrlwi r10,r11,28
	ctx.r10.u64 = ctx.r11.u32 & 0xF;
	// b 0x829558f8
	goto loc_829558F8;
loc_829558EC:
	// clrlwi r10,r9,28
	ctx.r10.u64 = ctx.r9.u32 & 0xF;
	// rlwinm r30,r9,28,4,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 28) & 0xFFFFFFF;
	// addi r31,r11,-4
	ctx.r31.s64 = ctx.r11.s64 + -4;
loc_829558F8:
	// lis r11,-32088
	ctx.r11.s64 = -2102919168;
	// rlwinm r7,r10,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r11,r11,-5952
	ctx.r11.s64 = ctx.r11.s64 + -5952;
	// lis r9,-32228
	ctx.r9.s64 = -2112094208;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// addi r9,r9,25568
	ctx.r9.s64 = ctx.r9.s64 + 25568;
	// lwzx r11,r7,r11
	ctx.r11.u64 = REX_LOAD_U32(ctx.r7.u32 + ctx.r11.u32);
	// stw r11,0(r4)
	REX_STORE_U32(ctx.r4.u32 + 0, ctx.r11.u32);
	// lbzx r11,r10,r9
	ctx.r11.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r9.u32);
	// stw r11,0(r5)
	REX_STORE_U32(ctx.r5.u32 + 0, ctx.r11.u32);
	// bne cr6,0x8295594c
	if (!ctx.cr6.eq) goto loc_8295594C;
	// li r11,0
	ctx.r11.s64 = 0;
loc_82955928:
	// stbx r11,r11,r28
	REX_STORE_U8(ctx.r11.u32 + ctx.r28.u32, ctx.r11.u8);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// cmplwi cr6,r11,16
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 16, ctx.xer);
	// blt cr6,0x82955928
	if (ctx.cr6.lt) goto loc_82955928;
	// stw r8,4(r27)
	REX_STORE_U32(ctx.r27.u32 + 4, ctx.r8.u32);
	// stw r30,0(r27)
	REX_STORE_U32(ctx.r27.u32 + 0, ctx.r30.u32);
	// stw r31,8(r27)
	REX_STORE_U32(ctx.r27.u32 + 8, ctx.r31.u32);
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
loc_8295594C:
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// bne cr6,0x82955968
	if (!ctx.cr6.eq) goto loc_82955968;
	// lwz r9,0(r8)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// rlwinm r10,r9,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x82955974
	goto loc_82955974;
loc_82955968:
	// mr r9,r30
	ctx.r9.u64 = ctx.r30.u64;
	// rlwinm r10,r30,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r31,-1
	ctx.r11.s64 = ctx.r31.s64 + -1;
loc_82955974:
	// clrlwi r9,r9,31
	ctx.r9.u64 = ctx.r9.u32 & 0x1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// bne cr6,0x82955fc4
	if (!ctx.cr6.eq) goto loc_82955FC4;
	// cmplwi cr6,r11,2
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 2, ctx.xer);
	// bge cr6,0x829559ac
	if (!ctx.cr6.lt) goto loc_829559AC;
	// lwz r9,0(r8)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subfic r7,r11,2
	ctx.xer.ca = ctx.r11.u32 <= 2;
	ctx.r7.u64 = static_cast<uint64_t>(2) - ctx.r11.u64;
	// addi r31,r11,30
	ctx.r31.s64 = ctx.r11.s64 + 30;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// slw r6,r9,r11
	ctx.r6.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r11.u8 & 0x3F));
	// srw r30,r9,r7
	ctx.r30.u64 = ctx.r7.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r7.u8 & 0x3F));
	// or r11,r6,r10
	ctx.r11.u64 = ctx.r6.u64 | ctx.r10.u64;
	// clrlwi r29,r11,30
	ctx.r29.u64 = ctx.r11.u32 & 0x3;
	// b 0x829559b8
	goto loc_829559B8;
loc_829559AC:
	// clrlwi r29,r10,30
	ctx.r29.u64 = ctx.r10.u32 & 0x3;
	// rlwinm r30,r10,30,2,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 30) & 0x3FFFFFFF;
	// addi r31,r11,-2
	ctx.r31.s64 = ctx.r11.s64 + -2;
loc_829559B8:
	// cmplwi cr6,r29,0
	ctx.cr6.compare<uint32_t>(ctx.r29.u32, 0, ctx.xer);
	// li r11,1
	ctx.r11.s64 = 1;
	// bne cr6,0x82955a38
	if (!ctx.cr6.eq) goto loc_82955A38;
	// addi r9,r28,-1
	ctx.r9.s64 = ctx.r28.s64 + -1;
loc_829559C8:
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// bne cr6,0x829559e4
	if (!ctx.cr6.eq) goto loc_829559E4;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// li r31,31
	ctx.r31.s64 = 31;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// rlwinm r30,r10,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x829559f0
	goto loc_829559F0;
loc_829559E4:
	// mr r10,r30
	ctx.r10.u64 = ctx.r30.u64;
	// rlwinm r30,r30,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
loc_829559F0:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// addi r10,r11,255
	ctx.r10.s64 = ctx.r11.s64 + 255;
	// beq cr6,0x82955a0c
	if (ctx.cr6.eq) goto loc_82955A0C;
	// stbx r11,r9,r11
	REX_STORE_U8(ctx.r9.u32 + ctx.r11.u32, ctx.r11.u8);
	// stbx r10,r11,r28
	REX_STORE_U8(ctx.r11.u32 + ctx.r28.u32, ctx.r10.u8);
	// b 0x82955a14
	goto loc_82955A14;
loc_82955A0C:
	// stbx r11,r11,r28
	REX_STORE_U8(ctx.r11.u32 + ctx.r28.u32, ctx.r11.u8);
	// stbx r10,r9,r11
	REX_STORE_U8(ctx.r9.u32 + ctx.r11.u32, ctx.r10.u8);
loc_82955A14:
	// addi r11,r11,2
	ctx.r11.s64 = ctx.r11.s64 + 2;
	// addi r10,r11,-1
	ctx.r10.s64 = ctx.r11.s64 + -1;
	// cmplwi cr6,r10,16
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 16, ctx.xer);
	// blt cr6,0x829559c8
	if (ctx.cr6.lt) goto loc_829559C8;
	// stw r8,4(r27)
	REX_STORE_U32(ctx.r27.u32 + 4, ctx.r8.u32);
	// stw r30,0(r27)
	REX_STORE_U32(ctx.r27.u32 + 0, ctx.r30.u32);
	// stw r31,8(r27)
	REX_STORE_U32(ctx.r27.u32 + 8, ctx.r31.u32);
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
loc_82955A38:
	// addi r10,r1,80
	ctx.r10.s64 = ctx.r1.s64 + 80;
	// li r9,0
	ctx.r9.s64 = 0;
	// addi r7,r10,-1
	ctx.r7.s64 = ctx.r10.s64 + -1;
loc_82955A44:
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// bne cr6,0x82955a60
	if (!ctx.cr6.eq) goto loc_82955A60;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// li r31,31
	ctx.r31.s64 = 31;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// rlwinm r30,r10,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x82955a6c
	goto loc_82955A6C;
loc_82955A60:
	// mr r10,r30
	ctx.r10.u64 = ctx.r30.u64;
	// rlwinm r30,r30,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
loc_82955A6C:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// addi r5,r1,80
	ctx.r5.s64 = ctx.r1.s64 + 80;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x82955a88
	if (ctx.cr6.eq) goto loc_82955A88;
	// stbx r11,r7,r11
	REX_STORE_U8(ctx.r7.u32 + ctx.r11.u32, ctx.r11.u8);
	// stbx r9,r11,r5
	REX_STORE_U8(ctx.r11.u32 + ctx.r5.u32, ctx.r9.u8);
	// b 0x82955a90
	goto loc_82955A90;
loc_82955A88:
	// stbx r9,r7,r11
	REX_STORE_U8(ctx.r7.u32 + ctx.r11.u32, ctx.r9.u8);
	// stbx r11,r11,r5
	REX_STORE_U8(ctx.r11.u32 + ctx.r5.u32, ctx.r11.u8);
loc_82955A90:
	// addi r11,r11,2
	ctx.r11.s64 = ctx.r11.s64 + 2;
	// addi r9,r9,2
	ctx.r9.s64 = ctx.r9.s64 + 2;
	// cmplwi cr6,r11,17
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 17, ctx.xer);
	// blt cr6,0x82955a44
	if (ctx.cr6.lt) goto loc_82955A44;
	// cmplwi cr6,r29,1
	ctx.cr6.compare<uint32_t>(ctx.r29.u32, 1, ctx.xer);
	// bne cr6,0x82955c4c
	if (!ctx.cr6.eq) goto loc_82955C4C;
	// cmplwi cr6,r31,3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 3, ctx.xer);
	// bge cr6,0x82955ac4
	if (!ctx.cr6.lt) goto loc_82955AC4;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,29
	ctx.r3.u64 = ctx.r11.u32 & 0x7;
	// b 0x82955ac8
	goto loc_82955AC8;
loc_82955AC4:
	// clrlwi r3,r30,29
	ctx.r3.u64 = ctx.r30.u32 & 0x7;
loc_82955AC8:
	// li r7,2
	ctx.r7.s64 = 2;
	// addi r6,r1,82
	ctx.r6.s64 = ctx.r1.s64 + 82;
	// addi r5,r1,80
	ctx.r5.s64 = ctx.r1.s64 + 80;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// bl 0x82955800
	ctx.lr = 0x82955ADC;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955b00
	if (!ctx.cr6.lt) goto loc_82955B00;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82955b08
	goto loc_82955B08;
loc_82955B00:
	// subf r31,r3,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r3.u64;
	// srw r30,r30,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r3.u8 & 0x3F));
loc_82955B08:
	// cmplwi cr6,r31,3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 3, ctx.xer);
	// bge cr6,0x82955b24
	if (!ctx.cr6.lt) goto loc_82955B24;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,29
	ctx.r3.u64 = ctx.r11.u32 & 0x7;
	// b 0x82955b28
	goto loc_82955B28;
loc_82955B24:
	// clrlwi r3,r30,29
	ctx.r3.u64 = ctx.r30.u32 & 0x7;
loc_82955B28:
	// li r7,2
	ctx.r7.s64 = 2;
	// addi r6,r1,86
	ctx.r6.s64 = ctx.r1.s64 + 86;
	// addi r5,r1,84
	ctx.r5.s64 = ctx.r1.s64 + 84;
	// addi r4,r28,4
	ctx.r4.s64 = ctx.r28.s64 + 4;
	// bl 0x82955800
	ctx.lr = 0x82955B3C;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955b60
	if (!ctx.cr6.lt) goto loc_82955B60;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82955b68
	goto loc_82955B68;
loc_82955B60:
	// subf r31,r3,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r3.u64;
	// srw r30,r30,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r3.u8 & 0x3F));
loc_82955B68:
	// cmplwi cr6,r31,3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 3, ctx.xer);
	// bge cr6,0x82955b84
	if (!ctx.cr6.lt) goto loc_82955B84;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,29
	ctx.r3.u64 = ctx.r11.u32 & 0x7;
	// b 0x82955b88
	goto loc_82955B88;
loc_82955B84:
	// clrlwi r3,r30,29
	ctx.r3.u64 = ctx.r30.u32 & 0x7;
loc_82955B88:
	// li r7,2
	ctx.r7.s64 = 2;
	// addi r6,r1,90
	ctx.r6.s64 = ctx.r1.s64 + 90;
	// addi r5,r1,88
	ctx.r5.s64 = ctx.r1.s64 + 88;
	// addi r4,r28,8
	ctx.r4.s64 = ctx.r28.s64 + 8;
	// bl 0x82955800
	ctx.lr = 0x82955B9C;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955bc0
	if (!ctx.cr6.lt) goto loc_82955BC0;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82955bc8
	goto loc_82955BC8;
loc_82955BC0:
	// subf r31,r3,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r3.u64;
	// srw r30,r30,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r3.u8 & 0x3F));
loc_82955BC8:
	// cmplwi cr6,r31,3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 3, ctx.xer);
	// bge cr6,0x82955be4
	if (!ctx.cr6.lt) goto loc_82955BE4;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,29
	ctx.r3.u64 = ctx.r11.u32 & 0x7;
	// b 0x82955be8
	goto loc_82955BE8;
loc_82955BE4:
	// clrlwi r3,r30,29
	ctx.r3.u64 = ctx.r30.u32 & 0x7;
loc_82955BE8:
	// li r7,2
	ctx.r7.s64 = 2;
	// addi r6,r1,94
	ctx.r6.s64 = ctx.r1.s64 + 94;
	// addi r5,r1,92
	ctx.r5.s64 = ctx.r1.s64 + 92;
	// addi r4,r28,12
	ctx.r4.s64 = ctx.r28.s64 + 12;
	// bl 0x82955800
	ctx.lr = 0x82955BFC;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955c30
	if (!ctx.cr6.lt) goto loc_82955C30;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// stw r8,4(r27)
	REX_STORE_U32(ctx.r27.u32 + 4, ctx.r8.u32);
	// stw r31,8(r27)
	REX_STORE_U32(ctx.r27.u32 + 8, ctx.r31.u32);
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// stw r30,0(r27)
	REX_STORE_U32(ctx.r27.u32 + 0, ctx.r30.u32);
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
loc_82955C30:
	// subf r31,r3,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r3.u64;
	// stw r8,4(r27)
	REX_STORE_U32(ctx.r27.u32 + 4, ctx.r8.u32);
	// srw r30,r30,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r3.u8 & 0x3F));
	// stw r31,8(r27)
	REX_STORE_U32(ctx.r27.u32 + 8, ctx.r31.u32);
	// stw r30,0(r27)
	REX_STORE_U32(ctx.r27.u32 + 0, ctx.r30.u32);
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
loc_82955C4C:
	// cmplwi cr6,r31,3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 3, ctx.xer);
	// bge cr6,0x82955c68
	if (!ctx.cr6.lt) goto loc_82955C68;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,29
	ctx.r3.u64 = ctx.r11.u32 & 0x7;
	// b 0x82955c6c
	goto loc_82955C6C;
loc_82955C68:
	// clrlwi r3,r30,29
	ctx.r3.u64 = ctx.r30.u32 & 0x7;
loc_82955C6C:
	// li r7,2
	ctx.r7.s64 = 2;
	// addi r6,r1,82
	ctx.r6.s64 = ctx.r1.s64 + 82;
	// addi r5,r1,80
	ctx.r5.s64 = ctx.r1.s64 + 80;
	// addi r4,r1,96
	ctx.r4.s64 = ctx.r1.s64 + 96;
	// bl 0x82955800
	ctx.lr = 0x82955C80;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955ca4
	if (!ctx.cr6.lt) goto loc_82955CA4;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82955cac
	goto loc_82955CAC;
loc_82955CA4:
	// subf r31,r3,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r3.u64;
	// srw r30,r30,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r3.u8 & 0x3F));
loc_82955CAC:
	// cmplwi cr6,r31,3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 3, ctx.xer);
	// bge cr6,0x82955cc8
	if (!ctx.cr6.lt) goto loc_82955CC8;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,29
	ctx.r3.u64 = ctx.r11.u32 & 0x7;
	// b 0x82955ccc
	goto loc_82955CCC;
loc_82955CC8:
	// clrlwi r3,r30,29
	ctx.r3.u64 = ctx.r30.u32 & 0x7;
loc_82955CCC:
	// li r7,2
	ctx.r7.s64 = 2;
	// addi r6,r1,86
	ctx.r6.s64 = ctx.r1.s64 + 86;
	// addi r5,r1,84
	ctx.r5.s64 = ctx.r1.s64 + 84;
	// addi r4,r1,100
	ctx.r4.s64 = ctx.r1.s64 + 100;
	// bl 0x82955800
	ctx.lr = 0x82955CE0;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955d04
	if (!ctx.cr6.lt) goto loc_82955D04;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82955d0c
	goto loc_82955D0C;
loc_82955D04:
	// subf r31,r3,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r3.u64;
	// srw r30,r30,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r3.u8 & 0x3F));
loc_82955D0C:
	// cmplwi cr6,r31,3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 3, ctx.xer);
	// bge cr6,0x82955d28
	if (!ctx.cr6.lt) goto loc_82955D28;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,29
	ctx.r3.u64 = ctx.r11.u32 & 0x7;
	// b 0x82955d2c
	goto loc_82955D2C;
loc_82955D28:
	// clrlwi r3,r30,29
	ctx.r3.u64 = ctx.r30.u32 & 0x7;
loc_82955D2C:
	// li r7,2
	ctx.r7.s64 = 2;
	// addi r6,r1,90
	ctx.r6.s64 = ctx.r1.s64 + 90;
	// addi r5,r1,88
	ctx.r5.s64 = ctx.r1.s64 + 88;
	// addi r4,r1,104
	ctx.r4.s64 = ctx.r1.s64 + 104;
	// bl 0x82955800
	ctx.lr = 0x82955D40;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955d64
	if (!ctx.cr6.lt) goto loc_82955D64;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82955d6c
	goto loc_82955D6C;
loc_82955D64:
	// subf r31,r3,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r3.u64;
	// srw r30,r30,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r3.u8 & 0x3F));
loc_82955D6C:
	// cmplwi cr6,r31,3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 3, ctx.xer);
	// bge cr6,0x82955d88
	if (!ctx.cr6.lt) goto loc_82955D88;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,29
	ctx.r3.u64 = ctx.r11.u32 & 0x7;
	// b 0x82955d8c
	goto loc_82955D8C;
loc_82955D88:
	// clrlwi r3,r30,29
	ctx.r3.u64 = ctx.r30.u32 & 0x7;
loc_82955D8C:
	// li r7,2
	ctx.r7.s64 = 2;
	// addi r6,r1,94
	ctx.r6.s64 = ctx.r1.s64 + 94;
	// addi r5,r1,92
	ctx.r5.s64 = ctx.r1.s64 + 92;
	// addi r4,r1,108
	ctx.r4.s64 = ctx.r1.s64 + 108;
	// bl 0x82955800
	ctx.lr = 0x82955DA0;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955dc4
	if (!ctx.cr6.lt) goto loc_82955DC4;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82955dcc
	goto loc_82955DCC;
loc_82955DC4:
	// subf r31,r3,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r3.u64;
	// srw r30,r30,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r3.u8 & 0x3F));
loc_82955DCC:
	// cmplwi cr6,r29,2
	ctx.cr6.compare<uint32_t>(ctx.r29.u32, 2, ctx.xer);
	// bne cr6,0x82955e9c
	if (!ctx.cr6.eq) goto loc_82955E9C;
	// cmplwi cr6,r31,7
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 7, ctx.xer);
	// bge cr6,0x82955df0
	if (!ctx.cr6.lt) goto loc_82955DF0;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,25
	ctx.r3.u64 = ctx.r11.u32 & 0x7F;
	// b 0x82955df4
	goto loc_82955DF4;
loc_82955DF0:
	// clrlwi r3,r30,25
	ctx.r3.u64 = ctx.r30.u32 & 0x7F;
loc_82955DF4:
	// li r7,4
	ctx.r7.s64 = 4;
	// addi r6,r1,100
	ctx.r6.s64 = ctx.r1.s64 + 100;
	// addi r5,r1,96
	ctx.r5.s64 = ctx.r1.s64 + 96;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// bl 0x82955800
	ctx.lr = 0x82955E08;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955e2c
	if (!ctx.cr6.lt) goto loc_82955E2C;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82955e34
	goto loc_82955E34;
loc_82955E2C:
	// subf r31,r3,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r3.u64;
	// srw r30,r30,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r3.u8 & 0x3F));
loc_82955E34:
	// cmplwi cr6,r31,7
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 7, ctx.xer);
	// bge cr6,0x82955e50
	if (!ctx.cr6.lt) goto loc_82955E50;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,25
	ctx.r3.u64 = ctx.r11.u32 & 0x7F;
	// b 0x82955e54
	goto loc_82955E54;
loc_82955E50:
	// clrlwi r3,r30,25
	ctx.r3.u64 = ctx.r30.u32 & 0x7F;
loc_82955E54:
	// li r7,4
	ctx.r7.s64 = 4;
	// addi r6,r1,108
	ctx.r6.s64 = ctx.r1.s64 + 108;
	// addi r5,r1,104
	ctx.r5.s64 = ctx.r1.s64 + 104;
	// addi r4,r28,8
	ctx.r4.s64 = ctx.r28.s64 + 8;
	// bl 0x82955800
	ctx.lr = 0x82955E68;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955c30
	if (!ctx.cr6.lt) goto loc_82955C30;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// stw r8,4(r27)
	REX_STORE_U32(ctx.r27.u32 + 4, ctx.r8.u32);
	// stw r31,8(r27)
	REX_STORE_U32(ctx.r27.u32 + 8, ctx.r31.u32);
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// stw r30,0(r27)
	REX_STORE_U32(ctx.r27.u32 + 0, ctx.r30.u32);
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
loc_82955E9C:
	// cmplwi cr6,r31,7
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 7, ctx.xer);
	// bge cr6,0x82955eb8
	if (!ctx.cr6.lt) goto loc_82955EB8;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,25
	ctx.r3.u64 = ctx.r11.u32 & 0x7F;
	// b 0x82955ebc
	goto loc_82955EBC;
loc_82955EB8:
	// clrlwi r3,r30,25
	ctx.r3.u64 = ctx.r30.u32 & 0x7F;
loc_82955EBC:
	// li r7,4
	ctx.r7.s64 = 4;
	// addi r6,r1,100
	ctx.r6.s64 = ctx.r1.s64 + 100;
	// addi r5,r1,96
	ctx.r5.s64 = ctx.r1.s64 + 96;
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// bl 0x82955800
	ctx.lr = 0x82955ED0;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955ef4
	if (!ctx.cr6.lt) goto loc_82955EF4;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82955efc
	goto loc_82955EFC;
loc_82955EF4:
	// subf r31,r3,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r3.u64;
	// srw r30,r30,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r3.u8 & 0x3F));
loc_82955EFC:
	// cmplwi cr6,r31,7
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 7, ctx.xer);
	// bge cr6,0x82955f18
	if (!ctx.cr6.lt) goto loc_82955F18;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,25
	ctx.r3.u64 = ctx.r11.u32 & 0x7F;
	// b 0x82955f1c
	goto loc_82955F1C;
loc_82955F18:
	// clrlwi r3,r30,25
	ctx.r3.u64 = ctx.r30.u32 & 0x7F;
loc_82955F1C:
	// li r7,4
	ctx.r7.s64 = 4;
	// addi r6,r1,108
	ctx.r6.s64 = ctx.r1.s64 + 108;
	// addi r5,r1,104
	ctx.r5.s64 = ctx.r1.s64 + 104;
	// addi r4,r1,88
	ctx.r4.s64 = ctx.r1.s64 + 88;
	// bl 0x82955800
	ctx.lr = 0x82955F30;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955f54
	if (!ctx.cr6.lt) goto loc_82955F54;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82955f5c
	goto loc_82955F5C;
loc_82955F54:
	// subf r31,r3,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r3.u64;
	// srw r30,r30,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r3.u8 & 0x3F));
loc_82955F5C:
	// cmplwi cr6,r31,15
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 15, ctx.xer);
	// bge cr6,0x82955f78
	if (!ctx.cr6.lt) goto loc_82955F78;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// slw r11,r11,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// clrlwi r3,r11,17
	ctx.r3.u64 = ctx.r11.u32 & 0x7FFF;
	// b 0x82955f7c
	goto loc_82955F7C;
loc_82955F78:
	// clrlwi r3,r30,17
	ctx.r3.u64 = ctx.r30.u32 & 0x7FFF;
loc_82955F7C:
	// li r7,8
	ctx.r7.s64 = 8;
	// addi r6,r1,88
	ctx.r6.s64 = ctx.r1.s64 + 88;
	// addi r5,r1,80
	ctx.r5.s64 = ctx.r1.s64 + 80;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// bl 0x82955800
	ctx.lr = 0x82955F90;
	sub_82955800(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bge cr6,0x82955c30
	if (!ctx.cr6.lt) goto loc_82955C30;
	// lwz r10,0(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subf r9,r31,r3
	ctx.r9.u64 = ctx.r3.u64 - ctx.r31.u64;
	// subf r11,r3,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r3.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// stw r8,4(r27)
	REX_STORE_U32(ctx.r27.u32 + 4, ctx.r8.u32);
	// stw r31,8(r27)
	REX_STORE_U32(ctx.r27.u32 + 8, ctx.r31.u32);
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// stw r30,0(r27)
	REX_STORE_U32(ctx.r27.u32 + 0, ctx.r30.u32);
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
loc_82955FC4:
	// cmplwi cr6,r11,3
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 3, ctx.xer);
	// bge cr6,0x82955ff0
	if (!ctx.cr6.lt) goto loc_82955FF0;
	// lwz r9,0(r8)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subfic r7,r11,3
	ctx.xer.ca = ctx.r11.u32 <= 3;
	ctx.r7.u64 = static_cast<uint64_t>(3) - ctx.r11.u64;
	// addi r31,r11,29
	ctx.r31.s64 = ctx.r11.s64 + 29;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// slw r6,r9,r11
	ctx.r6.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r11.u8 & 0x3F));
	// srw r30,r9,r7
	ctx.r30.u64 = ctx.r7.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r7.u8 & 0x3F));
	// or r11,r6,r10
	ctx.r11.u64 = ctx.r6.u64 | ctx.r10.u64;
	// clrlwi r6,r11,29
	ctx.r6.u64 = ctx.r11.u32 & 0x7;
	// b 0x82955ffc
	goto loc_82955FFC;
loc_82955FF0:
	// clrlwi r6,r10,29
	ctx.r6.u64 = ctx.r10.u32 & 0x7;
	// rlwinm r30,r10,29,3,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 29) & 0x1FFFFFFF;
	// addi r31,r11,-3
	ctx.r31.s64 = ctx.r11.s64 + -3;
loc_82955FFC:
	// lis r7,0
	ctx.r7.s64 = 0;
	// li r10,0
	ctx.r10.s64 = 0;
	// ori r7,r7,65535
	ctx.r7.u64 = ctx.r7.u64 | 65535;
	// li r5,1
	ctx.r5.s64 = 1;
loc_8295600C:
	// cmplwi cr6,r31,4
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 4, ctx.xer);
	// bge cr6,0x82956038
	if (!ctx.cr6.lt) goto loc_82956038;
	// lwz r9,0(r8)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// subfic r4,r31,4
	ctx.xer.ca = ctx.r31.u32 <= 4;
	ctx.r4.u64 = static_cast<uint64_t>(4) - ctx.r31.u64;
	// addi r8,r8,4
	ctx.r8.s64 = ctx.r8.s64 + 4;
	// slw r11,r9,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r31.u8 & 0x3F));
	// addi r31,r31,28
	ctx.r31.s64 = ctx.r31.s64 + 28;
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// srw r30,r9,r4
	ctx.r30.u64 = ctx.r4.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r4.u8 & 0x3F));
	// clrlwi r11,r11,28
	ctx.r11.u64 = ctx.r11.u32 & 0xF;
	// b 0x82956044
	goto loc_82956044;
loc_82956038:
	// clrlwi r11,r30,28
	ctx.r11.u64 = ctx.r30.u32 & 0xF;
	// rlwinm r30,r30,28,4,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 28) & 0xFFFFFFF;
	// addi r31,r31,-4
	ctx.r31.s64 = ctx.r31.s64 + -4;
loc_82956044:
	// stbx r11,r10,r28
	REX_STORE_U8(ctx.r10.u32 + ctx.r28.u32, ctx.r11.u8);
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
	// slw r11,r5,r11
	ctx.r11.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r5.u32 << (ctx.r11.u8 & 0x3F));
	// cmplw cr6,r10,r6
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r6.u32, ctx.xer);
	// andc r7,r7,r11
	ctx.r7.u64 = ctx.r7.u64 & ~ctx.r11.u64;
	// ble cr6,0x8295600c
	if (!ctx.cr6.gt) goto loc_8295600C;
	// li r11,0
	ctx.r11.s64 = 0;
	// add r10,r6,r28
	ctx.r10.u64 = ctx.r6.u64 + ctx.r28.u64;
loc_82956064:
	// clrlwi r9,r7,31
	ctx.r9.u64 = ctx.r7.u32 & 0x1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// beq cr6,0x82956078
	if (ctx.cr6.eq) goto loc_82956078;
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
	// stb r11,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r11.u8);
loc_82956078:
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// cmplwi cr6,r7,0
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 0, ctx.xer);
	// bne cr6,0x82956064
	if (!ctx.cr6.eq) goto loc_82956064;
	// stw r8,4(r27)
	REX_STORE_U32(ctx.r27.u32 + 4, ctx.r8.u32);
	// stw r30,0(r27)
	REX_STORE_U32(ctx.r27.u32 + 0, ctx.r30.u32);
	// stw r31,8(r27)
	REX_STORE_U32(ctx.r27.u32 + 8, ctx.r31.u32);
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_829560A0) {
	REX_FUNC_PROLOGUE(sub_829560A0);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x8241319c
	ctx.lr = 0x829560A8;
	__savegprlr_25(ctx, base);
	// stwu r1,-144(r1)
	ea = -144 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r8,r3
	ctx.r8.u64 = ctx.r3.u64;
	// mr r25,r4
	ctx.r25.u64 = ctx.r4.u64;
	// lwz r11,0(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// lwz r10,4(r8)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r8.u32 + 4);
	// cmplw cr6,r11,r10
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
	// bne cr6,0x82956374
	if (!ctx.cr6.eq) goto loc_82956374;
	// lwz r11,40(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 40);
	// li r9,-1
	ctx.r9.s64 = -1;
	// lwz r10,8(r25)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r25.u32 + 8);
	// lwz r6,0(r25)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r25.u32 + 0);
	// lwz r29,4(r25)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r25.u32 + 4);
	// cmplw cr6,r10,r11
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r11.u32, ctx.xer);
	// bge cr6,0x82956110
	if (!ctx.cr6.lt) goto loc_82956110;
	// lwz r7,0(r29)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subfic r5,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r5.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r4,r10,r11
	ctx.r4.u64 = ctx.r11.u64 - ctx.r10.u64;
	// subf r11,r11,r10
	ctx.r11.u64 = ctx.r10.u64 - ctx.r11.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// slw r10,r7,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r7.u32 << (ctx.r10.u8 & 0x3F));
	// srw r5,r9,r5
	ctx.r5.u64 = ctx.r5.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r5.u8 & 0x3F));
	// or r11,r10,r6
	ctx.r11.u64 = ctx.r10.u64 | ctx.r6.u64;
	// srw r30,r7,r4
	ctx.r30.u64 = ctx.r4.u8 & 0x20 ? 0 : (ctx.r7.u32 >> (ctx.r4.u8 & 0x3F));
	// and r5,r5,r11
	ctx.r5.u64 = ctx.r5.u64 & ctx.r11.u64;
	// b 0x82956124
	goto loc_82956124;
loc_82956110:
	// subfic r7,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r7.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r31,r11,r10
	ctx.r31.u64 = ctx.r10.u64 - ctx.r11.u64;
	// srw r30,r6,r11
	ctx.r30.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r6.u32 >> (ctx.r11.u8 & 0x3F));
	// srw r10,r9,r7
	ctx.r10.u64 = ctx.r7.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r7.u8 & 0x3F));
	// and r5,r10,r6
	ctx.r5.u64 = ctx.r10.u64 & ctx.r6.u64;
loc_82956124:
	// lwz r11,44(r8)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r8.u32 + 44);
	// cmplwi cr6,r5,0
	ctx.cr6.compare<uint32_t>(ctx.r5.u32, 0, ctx.xer);
	// beq cr6,0x8295635c
	if (ctx.cr6.eq) goto loc_8295635C;
	// add r10,r11,r5
	ctx.r10.u64 = ctx.r11.u64 + ctx.r5.u64;
	// stw r11,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r11.u32);
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// stw r10,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r10.u32);
	// bne cr6,0x82956158
	if (!ctx.cr6.eq) goto loc_82956158;
	// lwz r10,0(r29)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// li r31,31
	ctx.r31.s64 = 31;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// rlwinm r30,r10,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x82956164
	goto loc_82956164;
loc_82956158:
	// mr r10,r30
	ctx.r10.u64 = ctx.r30.u64;
	// rlwinm r30,r30,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
loc_82956164:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// bne cr6,0x829562f0
	if (!ctx.cr6.eq) goto loc_829562F0;
	// lwz r26,32(r8)
	ctx.r26.u64 = REX_LOAD_U32(ctx.r8.u32 + 32);
	// addi r3,r8,16
	ctx.r3.s64 = ctx.r8.s64 + 16;
	// lwz r4,36(r8)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r8.u32 + 36);
	// li r6,0
	ctx.r6.s64 = 0;
	// subfic r10,r26,32
	ctx.xer.ca = ctx.r26.u32 <= 32;
	ctx.r10.u64 = static_cast<uint64_t>(32) - ctx.r26.u64;
	// srw r28,r9,r10
	ctx.r28.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r10.u8 & 0x3F));
	// lis r10,-32228
	ctx.r10.s64 = -2112094208;
	// addi r27,r10,25584
	ctx.r27.s64 = ctx.r10.s64 + 25584;
loc_82956190:
	// cmplw cr6,r31,r26
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r26.u32, ctx.xer);
	// blt cr6,0x829561b8
	if (ctx.cr6.lt) goto loc_829561B8;
	// and r10,r28,r30
	ctx.r10.u64 = ctx.r28.u64 & ctx.r30.u64;
	// lbzx r10,r10,r4
	ctx.r10.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r4.u32);
	// clrlwi r9,r10,28
	ctx.r9.u64 = ctx.r10.u32 & 0xF;
	// rlwinm r10,r10,28,4,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 28) & 0xFFFFFFF;
	// subf r31,r10,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r10.u64;
	// lbzx r8,r9,r3
	ctx.r8.u64 = REX_LOAD_U8(ctx.r9.u32 + ctx.r3.u32);
	// srw r30,r30,r10
	ctx.r30.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r10.u8 & 0x3F));
	// b 0x82956200
	goto loc_82956200;
loc_829561B8:
	// lwz r9,0(r29)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// slw r10,r9,r31
	ctx.r10.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r31.u8 & 0x3F));
	// or r10,r10,r30
	ctx.r10.u64 = ctx.r10.u64 | ctx.r30.u64;
	// and r10,r10,r28
	ctx.r10.u64 = ctx.r10.u64 & ctx.r28.u64;
	// lbzx r10,r10,r4
	ctx.r10.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r4.u32);
	// clrlwi r8,r10,28
	ctx.r8.u64 = ctx.r10.u32 & 0xF;
	// rlwinm r10,r10,28,4,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 28) & 0xFFFFFFF;
	// cmplw cr6,r31,r10
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r10.u32, ctx.xer);
	// lbzx r8,r8,r3
	ctx.r8.u64 = REX_LOAD_U8(ctx.r8.u32 + ctx.r3.u32);
	// blt cr6,0x829561ec
	if (ctx.cr6.lt) goto loc_829561EC;
	// subf r31,r10,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r10.u64;
	// srw r30,r30,r10
	ctx.r30.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r10.u8 & 0x3F));
	// b 0x82956200
	goto loc_82956200;
loc_829561EC:
	// subf r7,r31,r10
	ctx.r7.u64 = ctx.r10.u64 - ctx.r31.u64;
	// subf r10,r10,r31
	ctx.r10.u64 = ctx.r31.u64 - ctx.r10.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// addi r31,r10,32
	ctx.r31.s64 = ctx.r10.s64 + 32;
	// srw r30,r9,r7
	ctx.r30.u64 = ctx.r7.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r7.u8 & 0x3F));
loc_82956200:
	// clrlwi r9,r8,24
	ctx.r9.u64 = ctx.r8.u32 & 0xFF;
	// cmplwi cr6,r9,12
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 12, ctx.xer);
	// blt cr6,0x829562c4
	if (ctx.cr6.lt) goto loc_829562C4;
	// rlwinm r10,r6,8,0,23
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// add r9,r9,r27
	ctx.r9.u64 = ctx.r9.u64 + ctx.r27.u64;
	// clrlwi r8,r11,30
	ctx.r8.u64 = ctx.r11.u32 & 0x3;
	// or r10,r10,r6
	ctx.r10.u64 = ctx.r10.u64 | ctx.r6.u64;
	// cmplwi cr6,r8,0
	ctx.cr6.compare<uint32_t>(ctx.r8.u32, 0, ctx.xer);
	// rlwinm r8,r10,16,0,15
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 16) & 0xFFFF0000;
	// lbz r9,-12(r9)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + -12);
	// or r8,r8,r10
	ctx.r8.u64 = ctx.r8.u64 | ctx.r10.u64;
	// mr r10,r9
	ctx.r10.u64 = ctx.r9.u64;
	// subf r5,r10,r5
	ctx.r5.u64 = ctx.r5.u64 - ctx.r10.u64;
	// beq cr6,0x829562a8
	if (ctx.cr6.eq) goto loc_829562A8;
	// clrlwi r7,r8,24
	ctx.r7.u64 = ctx.r8.u32 & 0xFF;
loc_8295623C:
	// addi r10,r10,255
	ctx.r10.s64 = ctx.r10.s64 + 255;
	// stb r7,0(r11)
	REX_STORE_U8(ctx.r11.u32 + 0, ctx.r7.u8);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// clrlwi r9,r10,24
	ctx.r9.u64 = ctx.r10.u32 & 0xFF;
	// mr r10,r9
	ctx.r10.u64 = ctx.r9.u64;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// bne cr6,0x8295623c
	if (!ctx.cr6.eq) goto loc_8295623C;
	// clrlwi r10,r9,24
	ctx.r10.u64 = ctx.r9.u32 & 0xFF;
	// cmplwi cr6,r10,4
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 4, ctx.xer);
	// blt cr6,0x82956280
	if (ctx.cr6.lt) goto loc_82956280;
loc_82956264:
	// addi r10,r10,252
	ctx.r10.s64 = ctx.r10.s64 + 252;
	// stw r8,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r8.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// clrlwi r9,r10,24
	ctx.r9.u64 = ctx.r10.u32 & 0xFF;
	// mr r10,r9
	ctx.r10.u64 = ctx.r9.u64;
	// cmplwi cr6,r10,4
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 4, ctx.xer);
	// bge cr6,0x82956264
	if (!ctx.cr6.lt) goto loc_82956264;
loc_82956280:
	// clrlwi r10,r9,24
	ctx.r10.u64 = ctx.r9.u32 & 0xFF;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x829562d4
	if (ctx.cr6.eq) goto loc_829562D4;
loc_8295628C:
	// addi r10,r10,255
	ctx.r10.s64 = ctx.r10.s64 + 255;
	// stb r7,0(r11)
	REX_STORE_U8(ctx.r11.u32 + 0, ctx.r7.u8);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// clrlwi r10,r10,24
	ctx.r10.u64 = ctx.r10.u32 & 0xFF;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// bne cr6,0x8295628c
	if (!ctx.cr6.eq) goto loc_8295628C;
	// b 0x829562d4
	goto loc_829562D4;
loc_829562A8:
	// addi r10,r10,252
	ctx.r10.s64 = ctx.r10.s64 + 252;
	// stw r8,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r8.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// clrlwi r10,r10,24
	ctx.r10.u64 = ctx.r10.u32 & 0xFF;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// bne cr6,0x829562a8
	if (!ctx.cr6.eq) goto loc_829562A8;
	// b 0x829562d4
	goto loc_829562D4;
loc_829562C4:
	// stb r8,0(r11)
	REX_STORE_U8(ctx.r11.u32 + 0, ctx.r8.u8);
	// mr r6,r9
	ctx.r6.u64 = ctx.r9.u64;
	// addi r5,r5,-1
	ctx.r5.s64 = ctx.r5.s64 + -1;
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
loc_829562D4:
	// cmplwi cr6,r5,0
	ctx.cr6.compare<uint32_t>(ctx.r5.u32, 0, ctx.xer);
	// bne cr6,0x82956190
	if (!ctx.cr6.eq) goto loc_82956190;
	// stw r29,4(r25)
	REX_STORE_U32(ctx.r25.u32 + 4, ctx.r29.u32);
	// stw r30,0(r25)
	REX_STORE_U32(ctx.r25.u32 + 0, ctx.r30.u32);
	// stw r31,8(r25)
	REX_STORE_U32(ctx.r25.u32 + 8, ctx.r31.u32);
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131ec
	__restgprlr_25(ctx, base);
	return;
loc_829562F0:
	// cmplwi cr6,r31,4
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 4, ctx.xer);
	// bge cr6,0x82956334
	if (!ctx.cr6.lt) goto loc_82956334;
	// lwz r10,0(r29)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subfic r9,r31,4
	ctx.xer.ca = ctx.r31.u32 <= 4;
	ctx.r9.u64 = static_cast<uint64_t>(4) - ctx.r31.u64;
	// mr r3,r11
	ctx.r3.u64 = ctx.r11.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// slw r8,r10,r31
	ctx.r8.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r10.u32 << (ctx.r31.u8 & 0x3F));
	// addi r31,r31,28
	ctx.r31.s64 = ctx.r31.s64 + 28;
	// or r8,r8,r30
	ctx.r8.u64 = ctx.r8.u64 | ctx.r30.u64;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// clrlwi r4,r8,28
	ctx.r4.u64 = ctx.r8.u32 & 0xF;
	// bl 0x82414530
	ctx.lr = 0x82956320;
	sub_82414530(ctx, base);
	// stw r29,4(r25)
	REX_STORE_U32(ctx.r25.u32 + 4, ctx.r29.u32);
	// stw r30,0(r25)
	REX_STORE_U32(ctx.r25.u32 + 0, ctx.r30.u32);
	// stw r31,8(r25)
	REX_STORE_U32(ctx.r25.u32 + 8, ctx.r31.u32);
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131ec
	__restgprlr_25(ctx, base);
	return;
loc_82956334:
	// clrlwi r4,r30,28
	ctx.r4.u64 = ctx.r30.u32 & 0xF;
	// mr r3,r11
	ctx.r3.u64 = ctx.r11.u64;
	// rlwinm r30,r30,28,4,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 28) & 0xFFFFFFF;
	// addi r31,r31,-4
	ctx.r31.s64 = ctx.r31.s64 + -4;
	// bl 0x82414530
	ctx.lr = 0x82956348;
	sub_82414530(ctx, base);
	// stw r29,4(r25)
	REX_STORE_U32(ctx.r25.u32 + 4, ctx.r29.u32);
	// stw r30,0(r25)
	REX_STORE_U32(ctx.r25.u32 + 0, ctx.r30.u32);
	// stw r31,8(r25)
	REX_STORE_U32(ctx.r25.u32 + 8, ctx.r31.u32);
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131ec
	__restgprlr_25(ctx, base);
	return;
loc_8295635C:
	// addi r10,r11,4
	ctx.r10.s64 = ctx.r11.s64 + 4;
	// stw r11,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r11.u32);
	// stw r10,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r10.u32);
	// stw r29,4(r25)
	REX_STORE_U32(ctx.r25.u32 + 4, ctx.r29.u32);
	// stw r30,0(r25)
	REX_STORE_U32(ctx.r25.u32 + 0, ctx.r30.u32);
	// stw r31,8(r25)
	REX_STORE_U32(ctx.r25.u32 + 8, ctx.r31.u32);
loc_82956374:
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131ec
	__restgprlr_25(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82956380) {
	REX_FUNC_PROLOGUE(sub_82956380);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413190
	ctx.lr = 0x82956388;
	__savegprlr_22(ctx, base);
	// stwu r1,-176(r1)
	ea = -176 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lwz r11,0(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);
	// mr r22,r4
	ctx.r22.u64 = ctx.r4.u64;
	// lwz r10,4(r3)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r3.u32 + 4);
	// mr r27,r5
	ctx.r27.u64 = ctx.r5.u64;
	// cmplw cr6,r11,r10
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
	// bne cr6,0x829565fc
	if (!ctx.cr6.eq) goto loc_829565FC;
	// lwz r11,40(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 40);
	// li r6,-1
	ctx.r6.s64 = -1;
	// lwz r10,8(r22)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r22.u32 + 8);
	// lwz r8,0(r22)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r22.u32 + 0);
	// lwz r28,4(r22)
	ctx.r28.u64 = REX_LOAD_U32(ctx.r22.u32 + 4);
	// cmplw cr6,r10,r11
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r11.u32, ctx.xer);
	// bge cr6,0x829563f0
	if (!ctx.cr6.lt) goto loc_829563F0;
	// lwz r9,0(r28)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r28.u32 + 0);
	// subfic r7,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r7.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r5,r10,r11
	ctx.r5.u64 = ctx.r11.u64 - ctx.r10.u64;
	// subf r11,r11,r10
	ctx.r11.u64 = ctx.r10.u64 - ctx.r11.u64;
	// addi r28,r28,4
	ctx.r28.s64 = ctx.r28.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// slw r10,r9,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r10.u8 & 0x3F));
	// srw r7,r6,r7
	ctx.r7.u64 = ctx.r7.u8 & 0x20 ? 0 : (ctx.r6.u32 >> (ctx.r7.u8 & 0x3F));
	// or r11,r10,r8
	ctx.r11.u64 = ctx.r10.u64 | ctx.r8.u64;
	// srw r30,r9,r5
	ctx.r30.u64 = ctx.r5.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r5.u8 & 0x3F));
	// and r25,r7,r11
	ctx.r25.u64 = ctx.r7.u64 & ctx.r11.u64;
	// b 0x82956404
	goto loc_82956404;
loc_829563F0:
	// subfic r9,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r9.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r31,r11,r10
	ctx.r31.u64 = ctx.r10.u64 - ctx.r11.u64;
	// srw r30,r8,r11
	ctx.r30.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r11.u8 & 0x3F));
	// srw r10,r6,r9
	ctx.r10.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r6.u32 >> (ctx.r9.u8 & 0x3F));
	// and r25,r10,r8
	ctx.r25.u64 = ctx.r10.u64 & ctx.r8.u64;
loc_82956404:
	// lwz r11,44(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 44);
	// cmpwi cr6,r25,0
	ctx.cr6.compare<int32_t>(ctx.r25.s32, 0, ctx.xer);
	// beq cr6,0x829565e4
	if (ctx.cr6.eq) goto loc_829565E4;
	// add r10,r11,r25
	ctx.r10.u64 = ctx.r11.u64 + ctx.r25.u64;
	// stw r11,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r11.u32);
	// lwz r23,32(r3)
	ctx.r23.u64 = REX_LOAD_U32(ctx.r3.u32 + 32);
	// addi r26,r3,16
	ctx.r26.s64 = ctx.r3.s64 + 16;
	// lwz r4,36(r3)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r3.u32 + 36);
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// mr r24,r11
	ctx.r24.u64 = ctx.r11.u64;
	// stw r10,4(r3)
	REX_STORE_U32(ctx.r3.u32 + 4, ctx.r10.u32);
	// lwz r29,384(r27)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r27.u32 + 384);
	// bne cr6,0x8295644c
	if (!ctx.cr6.eq) goto loc_8295644C;
	// lwz r11,0(r28)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r28.u32 + 0);
	// li r31,31
	ctx.r31.s64 = 31;
	// addi r28,r28,4
	ctx.r28.s64 = ctx.r28.s64 + 4;
	// rlwinm r30,r11,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x82956458
	goto loc_82956458;
loc_8295644C:
	// mr r11,r30
	ctx.r11.u64 = ctx.r30.u64;
	// rlwinm r30,r30,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
loc_82956458:
	// clrlwi r11,r11,31
	ctx.r11.u64 = ctx.r11.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82956468
	if (ctx.cr6.eq) goto loc_82956468;
	// subfic r25,r25,-20
	ctx.xer.ca = ctx.r25.u32 <= 4294967276;
	ctx.r25.u64 = static_cast<uint64_t>(-20) - ctx.r25.u64;
loc_82956468:
	// subfic r11,r23,32
	ctx.xer.ca = ctx.r23.u32 <= 32;
	ctx.r11.u64 = static_cast<uint64_t>(32) - ctx.r23.u64;
	// srw r5,r6,r11
	ctx.r5.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r6.u32 >> (ctx.r11.u8 & 0x3F));
loc_82956470:
	// addi r11,r29,64
	ctx.r11.s64 = ctx.r29.s64 + 64;
	// addi r10,r29,80
	ctx.r10.s64 = ctx.r29.s64 + 80;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r25,r25,-1
	ctx.r25.s64 = ctx.r25.s64 + -1;
	// lwzx r11,r11,r27
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + ctx.r27.u32);
	// lwzx r10,r10,r27
	ctx.r10.u64 = REX_LOAD_U32(ctx.r10.u32 + ctx.r27.u32);
	// cmplw cr6,r31,r11
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r11.u32, ctx.xer);
	// subfic r11,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r11.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// srw r11,r6,r11
	ctx.r11.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r6.u32 >> (ctx.r11.u8 & 0x3F));
	// blt cr6,0x829564c4
	if (ctx.cr6.lt) goto loc_829564C4;
	// and r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 & ctx.r30.u64;
	// rlwinm r9,r29,4,0,27
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 4) & 0xFFFFFFF0;
	// lbzx r11,r11,r10
	ctx.r11.u64 = REX_LOAD_U8(ctx.r11.u32 + ctx.r10.u32);
	// clrlwi r10,r11,28
	ctx.r10.u64 = ctx.r11.u32 & 0xF;
	// rlwinm r11,r11,28,4,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 28) & 0xFFFFFFF;
	// add r10,r10,r9
	ctx.r10.u64 = ctx.r10.u64 + ctx.r9.u64;
	// lbzx r29,r10,r27
	ctx.r29.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r27.u32);
	// srw r10,r30,r11
	ctx.r10.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r11.u8 & 0x3F));
	// subf r11,r11,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r11.u64;
	// b 0x82956510
	goto loc_82956510;
loc_829564C4:
	// lwz r9,0(r28)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r28.u32 + 0);
	// rlwinm r7,r29,4,0,27
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 4) & 0xFFFFFFF0;
	// slw r8,r9,r31
	ctx.r8.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r31.u8 & 0x3F));
	// or r8,r8,r30
	ctx.r8.u64 = ctx.r8.u64 | ctx.r30.u64;
	// and r11,r11,r8
	ctx.r11.u64 = ctx.r11.u64 & ctx.r8.u64;
	// lbzx r11,r11,r10
	ctx.r11.u64 = REX_LOAD_U8(ctx.r11.u32 + ctx.r10.u32);
	// clrlwi r8,r11,28
	ctx.r8.u64 = ctx.r11.u32 & 0xF;
	// rlwinm r10,r11,28,4,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 28) & 0xFFFFFFF;
	// add r11,r8,r7
	ctx.r11.u64 = ctx.r8.u64 + ctx.r7.u64;
	// cmplw cr6,r31,r10
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r10.u32, ctx.xer);
	// lbzx r29,r11,r27
	ctx.r29.u64 = REX_LOAD_U8(ctx.r11.u32 + ctx.r27.u32);
	// subf r11,r10,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r10.u64;
	// blt cr6,0x82956500
	if (ctx.cr6.lt) goto loc_82956500;
	// srw r10,r30,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r10.u8 & 0x3F));
	// b 0x82956510
	goto loc_82956510;
loc_82956500:
	// subf r8,r31,r10
	ctx.r8.u64 = ctx.r10.u64 - ctx.r31.u64;
	// addi r28,r28,4
	ctx.r28.s64 = ctx.r28.s64 + 4;
	// addi r11,r11,32
	ctx.r11.s64 = ctx.r11.s64 + 32;
	// srw r10,r9,r8
	ctx.r10.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r8.u8 & 0x3F));
loc_82956510:
	// cmplw cr6,r11,r23
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r23.u32, ctx.xer);
	// blt cr6,0x82956538
	if (ctx.cr6.lt) goto loc_82956538;
	// and r9,r5,r10
	ctx.r9.u64 = ctx.r5.u64 & ctx.r10.u64;
	// lbzx r9,r9,r4
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + ctx.r4.u32);
	// clrlwi r8,r9,28
	ctx.r8.u64 = ctx.r9.u32 & 0xF;
	// rlwinm r9,r9,28,4,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 28) & 0xFFFFFFF;
	// subf r31,r9,r11
	ctx.r31.u64 = ctx.r11.u64 - ctx.r9.u64;
	// lbzx r7,r8,r26
	ctx.r7.u64 = REX_LOAD_U8(ctx.r8.u32 + ctx.r26.u32);
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82956580
	goto loc_82956580;
loc_82956538:
	// lwz r8,0(r28)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r28.u32 + 0);
	// slw r9,r8,r11
	ctx.r9.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 << (ctx.r11.u8 & 0x3F));
	// or r9,r9,r10
	ctx.r9.u64 = ctx.r9.u64 | ctx.r10.u64;
	// and r9,r9,r5
	ctx.r9.u64 = ctx.r9.u64 & ctx.r5.u64;
	// lbzx r9,r9,r4
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + ctx.r4.u32);
	// clrlwi r7,r9,28
	ctx.r7.u64 = ctx.r9.u32 & 0xF;
	// rlwinm r9,r9,28,4,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 28) & 0xFFFFFFF;
	// cmplw cr6,r11,r9
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r9.u32, ctx.xer);
	// lbzx r7,r7,r26
	ctx.r7.u64 = REX_LOAD_U8(ctx.r7.u32 + ctx.r26.u32);
	// blt cr6,0x8295656c
	if (ctx.cr6.lt) goto loc_8295656C;
	// subf r31,r9,r11
	ctx.r31.u64 = ctx.r11.u64 - ctx.r9.u64;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82956580
	goto loc_82956580;
loc_8295656C:
	// subf r10,r11,r9
	ctx.r10.u64 = ctx.r9.u64 - ctx.r11.u64;
	// subf r11,r9,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r9.u64;
	// addi r28,r28,4
	ctx.r28.s64 = ctx.r28.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r8,r10
	ctx.r30.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r10.u8 & 0x3F));
loc_82956580:
	// rlwinm r11,r29,4,0,27
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 4) & 0xFFFFFFF0;
	// or r11,r11,r7
	ctx.r11.u64 = ctx.r11.u64 | ctx.r7.u64;
	// rlwinm r10,r11,0,24,24
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 0) & 0x80;
	// clrlwi r11,r11,25
	ctx.r11.u64 = ctx.r11.u32 & 0x7F;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x829565a0
	if (ctx.cr6.eq) goto loc_829565A0;
	// neg r11,r11
	ctx.r11.s64 = static_cast<int64_t>(-ctx.r11.u64);
	// clrlwi r11,r11,24
	ctx.r11.u64 = ctx.r11.u32 & 0xFF;
loc_829565A0:
	// addi r11,r11,128
	ctx.r11.s64 = ctx.r11.s64 + 128;
	// cmpwi cr6,r25,0
	ctx.cr6.compare<int32_t>(ctx.r25.s32, 0, ctx.xer);
	// stb r11,0(r24)
	REX_STORE_U8(ctx.r24.u32 + 0, ctx.r11.u8);
	// addi r24,r24,1
	ctx.r24.s64 = ctx.r24.s64 + 1;
	// bgt cr6,0x82956470
	if (ctx.cr6.gt) goto loc_82956470;
	// cmpwi cr6,r25,-22
	ctx.cr6.compare<int32_t>(ctx.r25.s32, -22, ctx.xer);
	// bge cr6,0x829565cc
	if (!ctx.cr6.lt) goto loc_829565CC;
	// lwz r3,44(r3)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r3.u32 + 44);
	// subfic r5,r25,-21
	ctx.xer.ca = ctx.r25.u32 <= 4294967275;
	ctx.r5.u64 = static_cast<uint64_t>(-21) - ctx.r25.u64;
	// lbz r4,0(r3)
	ctx.r4.u64 = REX_LOAD_U8(ctx.r3.u32 + 0);
	// bl 0x82414530
	ctx.lr = 0x829565CC;
	sub_82414530(ctx, base);
loc_829565CC:
	// stw r29,384(r27)
	REX_STORE_U32(ctx.r27.u32 + 384, ctx.r29.u32);
	// stw r28,4(r22)
	REX_STORE_U32(ctx.r22.u32 + 4, ctx.r28.u32);
	// stw r30,0(r22)
	REX_STORE_U32(ctx.r22.u32 + 0, ctx.r30.u32);
	// stw r31,8(r22)
	REX_STORE_U32(ctx.r22.u32 + 8, ctx.r31.u32);
	// addi r1,r1,176
	ctx.r1.s64 = ctx.r1.s64 + 176;
	// b 0x824131e0
	__restgprlr_22(ctx, base);
	return;
loc_829565E4:
	// addi r10,r11,4
	ctx.r10.s64 = ctx.r11.s64 + 4;
	// stw r11,4(r3)
	REX_STORE_U32(ctx.r3.u32 + 4, ctx.r11.u32);
	// stw r10,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r10.u32);
	// stw r28,4(r22)
	REX_STORE_U32(ctx.r22.u32 + 4, ctx.r28.u32);
	// stw r30,0(r22)
	REX_STORE_U32(ctx.r22.u32 + 0, ctx.r30.u32);
	// stw r31,8(r22)
	REX_STORE_U32(ctx.r22.u32 + 8, ctx.r31.u32);
loc_829565FC:
	// addi r1,r1,176
	ctx.r1.s64 = ctx.r1.s64 + 176;
	// b 0x824131e0
	__restgprlr_22(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82956608) {
	REX_FUNC_PROLOGUE(sub_82956608);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x8241318c
	ctx.lr = 0x82956610;
	__savegprlr_21(ctx, base);
	// stwu r1,-176(r1)
	ea = -176 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lwz r11,0(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);
	// mr r21,r4
	ctx.r21.u64 = ctx.r4.u64;
	// lwz r10,4(r3)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r3.u32 + 4);
	// mr r28,r5
	ctx.r28.u64 = ctx.r5.u64;
	// cmplw cr6,r11,r10
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
	// bne cr6,0x8295686c
	if (!ctx.cr6.eq) goto loc_8295686C;
	// lwz r11,40(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 40);
	// li r5,-1
	ctx.r5.s64 = -1;
	// lwz r10,8(r21)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r21.u32 + 8);
	// lwz r8,0(r21)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r21.u32 + 0);
	// lwz r29,4(r21)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r21.u32 + 4);
	// cmplw cr6,r10,r11
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r11.u32, ctx.xer);
	// bge cr6,0x82956678
	if (!ctx.cr6.lt) goto loc_82956678;
	// lwz r9,0(r29)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subfic r7,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r7.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r6,r10,r11
	ctx.r6.u64 = ctx.r11.u64 - ctx.r10.u64;
	// subf r11,r11,r10
	ctx.r11.u64 = ctx.r10.u64 - ctx.r11.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// slw r10,r9,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r10.u8 & 0x3F));
	// srw r7,r5,r7
	ctx.r7.u64 = ctx.r7.u8 & 0x20 ? 0 : (ctx.r5.u32 >> (ctx.r7.u8 & 0x3F));
	// or r11,r10,r8
	ctx.r11.u64 = ctx.r10.u64 | ctx.r8.u64;
	// srw r30,r9,r6
	ctx.r30.u64 = ctx.r6.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r6.u8 & 0x3F));
	// and r24,r7,r11
	ctx.r24.u64 = ctx.r7.u64 & ctx.r11.u64;
	// b 0x8295668c
	goto loc_8295668C;
loc_82956678:
	// subfic r9,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r9.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r31,r11,r10
	ctx.r31.u64 = ctx.r10.u64 - ctx.r11.u64;
	// srw r30,r8,r11
	ctx.r30.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r11.u8 & 0x3F));
	// srw r10,r5,r9
	ctx.r10.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r5.u32 >> (ctx.r9.u8 & 0x3F));
	// and r24,r10,r8
	ctx.r24.u64 = ctx.r10.u64 & ctx.r8.u64;
loc_8295668C:
	// lwz r11,44(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 44);
	// cmpwi cr6,r24,0
	ctx.cr6.compare<int32_t>(ctx.r24.s32, 0, ctx.xer);
	// beq cr6,0x82956854
	if (ctx.cr6.eq) goto loc_82956854;
	// add r10,r11,r24
	ctx.r10.u64 = ctx.r11.u64 + ctx.r24.u64;
	// stw r11,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r11.u32);
	// lwz r22,32(r3)
	ctx.r22.u64 = REX_LOAD_U32(ctx.r3.u32 + 32);
	// addi r25,r3,16
	ctx.r25.s64 = ctx.r3.s64 + 16;
	// lwz r26,36(r3)
	ctx.r26.u64 = REX_LOAD_U32(ctx.r3.u32 + 36);
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// mr r23,r11
	ctx.r23.u64 = ctx.r11.u64;
	// stw r10,4(r3)
	REX_STORE_U32(ctx.r3.u32 + 4, ctx.r10.u32);
	// lwz r7,384(r28)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r28.u32 + 384);
	// bne cr6,0x829566d4
	if (!ctx.cr6.eq) goto loc_829566D4;
	// lwz r11,0(r29)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// li r31,31
	ctx.r31.s64 = 31;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// rlwinm r30,r11,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x829566e0
	goto loc_829566E0;
loc_829566D4:
	// mr r11,r30
	ctx.r11.u64 = ctx.r30.u64;
	// rlwinm r30,r30,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
loc_829566E0:
	// clrlwi r11,r11,31
	ctx.r11.u64 = ctx.r11.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x829566f0
	if (ctx.cr6.eq) goto loc_829566F0;
	// subfic r24,r24,-20
	ctx.xer.ca = ctx.r24.u32 <= 4294967276;
	ctx.r24.u64 = static_cast<uint64_t>(-20) - ctx.r24.u64;
loc_829566F0:
	// subfic r11,r22,32
	ctx.xer.ca = ctx.r22.u32 <= 32;
	ctx.r11.u64 = static_cast<uint64_t>(32) - ctx.r22.u64;
	// srw r4,r5,r11
	ctx.r4.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r5.u32 >> (ctx.r11.u8 & 0x3F));
loc_829566F8:
	// addi r11,r7,64
	ctx.r11.s64 = ctx.r7.s64 + 64;
	// addi r10,r7,80
	ctx.r10.s64 = ctx.r7.s64 + 80;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r24,r24,-1
	ctx.r24.s64 = ctx.r24.s64 + -1;
	// lwzx r11,r11,r28
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + ctx.r28.u32);
	// lwzx r10,r10,r28
	ctx.r10.u64 = REX_LOAD_U32(ctx.r10.u32 + ctx.r28.u32);
	// cmplw cr6,r31,r11
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r11.u32, ctx.xer);
	// subfic r11,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r11.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// srw r11,r5,r11
	ctx.r11.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r5.u32 >> (ctx.r11.u8 & 0x3F));
	// blt cr6,0x8295674c
	if (ctx.cr6.lt) goto loc_8295674C;
	// and r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 & ctx.r30.u64;
	// rlwinm r9,r7,4,0,27
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 4) & 0xFFFFFFF0;
	// lbzx r11,r11,r10
	ctx.r11.u64 = REX_LOAD_U8(ctx.r11.u32 + ctx.r10.u32);
	// clrlwi r10,r11,28
	ctx.r10.u64 = ctx.r11.u32 & 0xF;
	// rlwinm r11,r11,28,4,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 28) & 0xFFFFFFF;
	// add r10,r10,r9
	ctx.r10.u64 = ctx.r10.u64 + ctx.r9.u64;
	// lbzx r27,r10,r28
	ctx.r27.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r28.u32);
	// srw r10,r30,r11
	ctx.r10.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r11.u8 & 0x3F));
	// subf r11,r11,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r11.u64;
	// b 0x82956798
	goto loc_82956798;
loc_8295674C:
	// lwz r9,0(r29)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// rlwinm r7,r7,4,0,27
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 4) & 0xFFFFFFF0;
	// slw r8,r9,r31
	ctx.r8.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r31.u8 & 0x3F));
	// or r8,r8,r30
	ctx.r8.u64 = ctx.r8.u64 | ctx.r30.u64;
	// and r11,r11,r8
	ctx.r11.u64 = ctx.r11.u64 & ctx.r8.u64;
	// lbzx r11,r11,r10
	ctx.r11.u64 = REX_LOAD_U8(ctx.r11.u32 + ctx.r10.u32);
	// clrlwi r8,r11,28
	ctx.r8.u64 = ctx.r11.u32 & 0xF;
	// rlwinm r10,r11,28,4,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 28) & 0xFFFFFFF;
	// add r11,r8,r7
	ctx.r11.u64 = ctx.r8.u64 + ctx.r7.u64;
	// cmplw cr6,r31,r10
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r10.u32, ctx.xer);
	// lbzx r27,r11,r28
	ctx.r27.u64 = REX_LOAD_U8(ctx.r11.u32 + ctx.r28.u32);
	// subf r11,r10,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r10.u64;
	// blt cr6,0x82956788
	if (ctx.cr6.lt) goto loc_82956788;
	// srw r10,r30,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r10.u8 & 0x3F));
	// b 0x82956798
	goto loc_82956798;
loc_82956788:
	// subf r8,r31,r10
	ctx.r8.u64 = ctx.r10.u64 - ctx.r31.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// addi r11,r11,32
	ctx.r11.s64 = ctx.r11.s64 + 32;
	// srw r10,r9,r8
	ctx.r10.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r8.u8 & 0x3F));
loc_82956798:
	// mr r7,r27
	ctx.r7.u64 = ctx.r27.u64;
	// cmplw cr6,r11,r22
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r22.u32, ctx.xer);
	// blt cr6,0x829567c4
	if (ctx.cr6.lt) goto loc_829567C4;
	// and r9,r4,r10
	ctx.r9.u64 = ctx.r4.u64 & ctx.r10.u64;
	// lbzx r9,r9,r26
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + ctx.r26.u32);
	// clrlwi r8,r9,28
	ctx.r8.u64 = ctx.r9.u32 & 0xF;
	// rlwinm r9,r9,28,4,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 28) & 0xFFFFFFF;
	// subf r31,r9,r11
	ctx.r31.u64 = ctx.r11.u64 - ctx.r9.u64;
	// lbzx r6,r8,r25
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + ctx.r25.u32);
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x8295680c
	goto loc_8295680C;
loc_829567C4:
	// lwz r8,0(r29)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// slw r9,r8,r11
	ctx.r9.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 << (ctx.r11.u8 & 0x3F));
	// or r9,r9,r10
	ctx.r9.u64 = ctx.r9.u64 | ctx.r10.u64;
	// and r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 & ctx.r4.u64;
	// lbzx r9,r9,r26
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + ctx.r26.u32);
	// clrlwi r6,r9,28
	ctx.r6.u64 = ctx.r9.u32 & 0xF;
	// rlwinm r9,r9,28,4,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 28) & 0xFFFFFFF;
	// cmplw cr6,r11,r9
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r9.u32, ctx.xer);
	// lbzx r6,r6,r25
	ctx.r6.u64 = REX_LOAD_U8(ctx.r6.u32 + ctx.r25.u32);
	// blt cr6,0x829567f8
	if (ctx.cr6.lt) goto loc_829567F8;
	// subf r31,r9,r11
	ctx.r31.u64 = ctx.r11.u64 - ctx.r9.u64;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x8295680c
	goto loc_8295680C;
loc_829567F8:
	// subf r10,r11,r9
	ctx.r10.u64 = ctx.r9.u64 - ctx.r11.u64;
	// subf r11,r9,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r9.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r8,r10
	ctx.r30.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r10.u8 & 0x3F));
loc_8295680C:
	// rlwinm r11,r27,4,0,27
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r27.u32 | (ctx.r27.u64 << 32), 4) & 0xFFFFFFF0;
	// cmpwi cr6,r24,0
	ctx.cr6.compare<int32_t>(ctx.r24.s32, 0, ctx.xer);
	// or r11,r11,r6
	ctx.r11.u64 = ctx.r11.u64 | ctx.r6.u64;
	// stb r11,0(r23)
	REX_STORE_U8(ctx.r23.u32 + 0, ctx.r11.u8);
	// addi r23,r23,1
	ctx.r23.s64 = ctx.r23.s64 + 1;
	// bgt cr6,0x829566f8
	if (ctx.cr6.gt) goto loc_829566F8;
	// cmpwi cr6,r24,-22
	ctx.cr6.compare<int32_t>(ctx.r24.s32, -22, ctx.xer);
	// bge cr6,0x8295683c
	if (!ctx.cr6.lt) goto loc_8295683C;
	// lwz r3,44(r3)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r3.u32 + 44);
	// subfic r5,r24,-21
	ctx.xer.ca = ctx.r24.u32 <= 4294967275;
	ctx.r5.u64 = static_cast<uint64_t>(-21) - ctx.r24.u64;
	// lbz r4,0(r3)
	ctx.r4.u64 = REX_LOAD_U8(ctx.r3.u32 + 0);
	// bl 0x82414530
	ctx.lr = 0x8295683C;
	sub_82414530(ctx, base);
loc_8295683C:
	// stw r27,384(r28)
	REX_STORE_U32(ctx.r28.u32 + 384, ctx.r27.u32);
	// stw r29,4(r21)
	REX_STORE_U32(ctx.r21.u32 + 4, ctx.r29.u32);
	// stw r30,0(r21)
	REX_STORE_U32(ctx.r21.u32 + 0, ctx.r30.u32);
	// stw r31,8(r21)
	REX_STORE_U32(ctx.r21.u32 + 8, ctx.r31.u32);
	// addi r1,r1,176
	ctx.r1.s64 = ctx.r1.s64 + 176;
	// b 0x824131dc
	__restgprlr_21(ctx, base);
	return;
loc_82956854:
	// addi r10,r11,4
	ctx.r10.s64 = ctx.r11.s64 + 4;
	// stw r11,4(r3)
	REX_STORE_U32(ctx.r3.u32 + 4, ctx.r11.u32);
	// stw r10,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r10.u32);
	// stw r29,4(r21)
	REX_STORE_U32(ctx.r21.u32 + 4, ctx.r29.u32);
	// stw r30,0(r21)
	REX_STORE_U32(ctx.r21.u32 + 0, ctx.r30.u32);
	// stw r31,8(r21)
	REX_STORE_U32(ctx.r21.u32 + 8, ctx.r31.u32);
loc_8295686C:
	// addi r1,r1,176
	ctx.r1.s64 = ctx.r1.s64 + 176;
	// b 0x824131dc
	__restgprlr_21(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82956878) {
	REX_FUNC_PROLOGUE(sub_82956878);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a8
	ctx.lr = 0x82956880;
	__savegprlr_28(ctx, base);
	// stwu r1,-128(r1)
	ea = -128 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r6,r3
	ctx.r6.u64 = ctx.r3.u64;
	// mr r28,r4
	ctx.r28.u64 = ctx.r4.u64;
	// lwz r11,0(r6)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// lwz r10,4(r6)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r6.u32 + 4);
	// cmplw cr6,r11,r10
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
	// bne cr6,0x82956a80
	if (!ctx.cr6.eq) goto loc_82956A80;
	// lwz r11,40(r6)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r6.u32 + 40);
	// li r9,-1
	ctx.r9.s64 = -1;
	// lwz r10,8(r28)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r28.u32 + 8);
	// lwz r7,0(r28)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r28.u32 + 0);
	// lwz r29,4(r28)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r28.u32 + 4);
	// cmplw cr6,r10,r11
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r11.u32, ctx.xer);
	// bge cr6,0x829568e8
	if (!ctx.cr6.lt) goto loc_829568E8;
	// lwz r8,0(r29)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subfic r5,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r5.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r4,r10,r11
	ctx.r4.u64 = ctx.r11.u64 - ctx.r10.u64;
	// subf r11,r11,r10
	ctx.r11.u64 = ctx.r10.u64 - ctx.r11.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// slw r10,r8,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r8.u32 << (ctx.r10.u8 & 0x3F));
	// srw r5,r9,r5
	ctx.r5.u64 = ctx.r5.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r5.u8 & 0x3F));
	// or r11,r10,r7
	ctx.r11.u64 = ctx.r10.u64 | ctx.r7.u64;
	// srw r30,r8,r4
	ctx.r30.u64 = ctx.r4.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r4.u8 & 0x3F));
	// and r5,r5,r11
	ctx.r5.u64 = ctx.r5.u64 & ctx.r11.u64;
	// b 0x829568fc
	goto loc_829568FC;
loc_829568E8:
	// subfic r8,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r8.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r31,r11,r10
	ctx.r31.u64 = ctx.r10.u64 - ctx.r11.u64;
	// srw r30,r7,r11
	ctx.r30.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r7.u32 >> (ctx.r11.u8 & 0x3F));
	// srw r10,r9,r8
	ctx.r10.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r8.u8 & 0x3F));
	// and r5,r10,r7
	ctx.r5.u64 = ctx.r10.u64 & ctx.r7.u64;
loc_829568FC:
	// lwz r11,44(r6)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r6.u32 + 44);
	// cmplwi cr6,r5,0
	ctx.cr6.compare<uint32_t>(ctx.r5.u32, 0, ctx.xer);
	// beq cr6,0x82956a68
	if (ctx.cr6.eq) goto loc_82956A68;
	// add r10,r11,r5
	ctx.r10.u64 = ctx.r11.u64 + ctx.r5.u64;
	// stw r11,0(r6)
	REX_STORE_U32(ctx.r6.u32 + 0, ctx.r11.u32);
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// stw r10,4(r6)
	REX_STORE_U32(ctx.r6.u32 + 4, ctx.r10.u32);
	// bne cr6,0x82956930
	if (!ctx.cr6.eq) goto loc_82956930;
	// lwz r10,0(r29)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// li r31,31
	ctx.r31.s64 = 31;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// rlwinm r30,r10,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295693c
	goto loc_8295693C;
loc_82956930:
	// mr r10,r30
	ctx.r10.u64 = ctx.r30.u64;
	// rlwinm r30,r30,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
loc_8295693C:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// bne cr6,0x829569fc
	if (!ctx.cr6.eq) goto loc_829569FC;
	// lwz r4,32(r6)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r6.u32 + 32);
	// addi r7,r6,16
	ctx.r7.s64 = ctx.r6.s64 + 16;
	// lwz r8,36(r6)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r6.u32 + 36);
	// subfic r10,r4,32
	ctx.xer.ca = ctx.r4.u32 <= 32;
	ctx.r10.u64 = static_cast<uint64_t>(32) - ctx.r4.u64;
	// srw r6,r9,r10
	ctx.r6.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r10.u8 & 0x3F));
loc_8295695C:
	// addi r5,r5,-1
	ctx.r5.s64 = ctx.r5.s64 + -1;
	// cmplw cr6,r31,r4
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r4.u32, ctx.xer);
	// blt cr6,0x82956990
	if (ctx.cr6.lt) goto loc_82956990;
	// and r10,r6,r30
	ctx.r10.u64 = ctx.r6.u64 & ctx.r30.u64;
	// lbzx r10,r10,r8
	ctx.r10.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r8.u32);
	// clrlwi r9,r10,28
	ctx.r9.u64 = ctx.r10.u32 & 0xF;
	// rlwinm r10,r10,28,4,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 28) & 0xFFFFFFF;
	// subf r31,r10,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r10.u64;
	// lbzx r9,r9,r7
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + ctx.r7.u32);
	// stb r9,0(r11)
	REX_STORE_U8(ctx.r11.u32 + 0, ctx.r9.u8);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// srw r30,r30,r10
	ctx.r30.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r10.u8 & 0x3F));
	// b 0x829569e0
	goto loc_829569E0;
loc_82956990:
	// lwz r9,0(r29)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// slw r10,r9,r31
	ctx.r10.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r31.u8 & 0x3F));
	// or r10,r10,r30
	ctx.r10.u64 = ctx.r10.u64 | ctx.r30.u64;
	// and r10,r10,r6
	ctx.r10.u64 = ctx.r10.u64 & ctx.r6.u64;
	// lbzx r10,r10,r8
	ctx.r10.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r8.u32);
	// clrlwi r3,r10,28
	ctx.r3.u64 = ctx.r10.u32 & 0xF;
	// rlwinm r10,r10,28,4,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 28) & 0xFFFFFFF;
	// cmplw cr6,r31,r10
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r10.u32, ctx.xer);
	// lbzx r3,r3,r7
	ctx.r3.u64 = REX_LOAD_U8(ctx.r3.u32 + ctx.r7.u32);
	// stb r3,0(r11)
	REX_STORE_U8(ctx.r11.u32 + 0, ctx.r3.u8);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// blt cr6,0x829569cc
	if (ctx.cr6.lt) goto loc_829569CC;
	// subf r31,r10,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r10.u64;
	// srw r30,r30,r10
	ctx.r30.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r10.u8 & 0x3F));
	// b 0x829569e0
	goto loc_829569E0;
loc_829569CC:
	// subf r3,r31,r10
	ctx.r3.u64 = ctx.r10.u64 - ctx.r31.u64;
	// subf r10,r10,r31
	ctx.r10.u64 = ctx.r31.u64 - ctx.r10.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// addi r31,r10,32
	ctx.r31.s64 = ctx.r10.s64 + 32;
	// srw r30,r9,r3
	ctx.r30.u64 = ctx.r3.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r3.u8 & 0x3F));
loc_829569E0:
	// cmplwi cr6,r5,0
	ctx.cr6.compare<uint32_t>(ctx.r5.u32, 0, ctx.xer);
	// bne cr6,0x8295695c
	if (!ctx.cr6.eq) goto loc_8295695C;
	// stw r29,4(r28)
	REX_STORE_U32(ctx.r28.u32 + 4, ctx.r29.u32);
	// stw r30,0(r28)
	REX_STORE_U32(ctx.r28.u32 + 0, ctx.r30.u32);
	// stw r31,8(r28)
	REX_STORE_U32(ctx.r28.u32 + 8, ctx.r31.u32);
	// addi r1,r1,128
	ctx.r1.s64 = ctx.r1.s64 + 128;
	// b 0x824131f8
	__restgprlr_28(ctx, base);
	return;
loc_829569FC:
	// cmplwi cr6,r31,4
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 4, ctx.xer);
	// bge cr6,0x82956a40
	if (!ctx.cr6.lt) goto loc_82956A40;
	// lwz r10,0(r29)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subfic r9,r31,4
	ctx.xer.ca = ctx.r31.u32 <= 4;
	ctx.r9.u64 = static_cast<uint64_t>(4) - ctx.r31.u64;
	// mr r3,r11
	ctx.r3.u64 = ctx.r11.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// slw r8,r10,r31
	ctx.r8.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r10.u32 << (ctx.r31.u8 & 0x3F));
	// addi r31,r31,28
	ctx.r31.s64 = ctx.r31.s64 + 28;
	// or r8,r8,r30
	ctx.r8.u64 = ctx.r8.u64 | ctx.r30.u64;
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// clrlwi r4,r8,28
	ctx.r4.u64 = ctx.r8.u32 & 0xF;
	// bl 0x82414530
	ctx.lr = 0x82956A2C;
	sub_82414530(ctx, base);
	// stw r29,4(r28)
	REX_STORE_U32(ctx.r28.u32 + 4, ctx.r29.u32);
	// stw r30,0(r28)
	REX_STORE_U32(ctx.r28.u32 + 0, ctx.r30.u32);
	// stw r31,8(r28)
	REX_STORE_U32(ctx.r28.u32 + 8, ctx.r31.u32);
	// addi r1,r1,128
	ctx.r1.s64 = ctx.r1.s64 + 128;
	// b 0x824131f8
	__restgprlr_28(ctx, base);
	return;
loc_82956A40:
	// clrlwi r4,r30,28
	ctx.r4.u64 = ctx.r30.u32 & 0xF;
	// mr r3,r11
	ctx.r3.u64 = ctx.r11.u64;
	// rlwinm r30,r30,28,4,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 28) & 0xFFFFFFF;
	// addi r31,r31,-4
	ctx.r31.s64 = ctx.r31.s64 + -4;
	// bl 0x82414530
	ctx.lr = 0x82956A54;
	sub_82414530(ctx, base);
	// stw r29,4(r28)
	REX_STORE_U32(ctx.r28.u32 + 4, ctx.r29.u32);
	// stw r30,0(r28)
	REX_STORE_U32(ctx.r28.u32 + 0, ctx.r30.u32);
	// stw r31,8(r28)
	REX_STORE_U32(ctx.r28.u32 + 8, ctx.r31.u32);
	// addi r1,r1,128
	ctx.r1.s64 = ctx.r1.s64 + 128;
	// b 0x824131f8
	__restgprlr_28(ctx, base);
	return;
loc_82956A68:
	// addi r10,r11,4
	ctx.r10.s64 = ctx.r11.s64 + 4;
	// stw r11,4(r6)
	REX_STORE_U32(ctx.r6.u32 + 4, ctx.r11.u32);
	// stw r10,0(r6)
	REX_STORE_U32(ctx.r6.u32 + 0, ctx.r10.u32);
	// stw r29,4(r28)
	REX_STORE_U32(ctx.r28.u32 + 4, ctx.r29.u32);
	// stw r30,0(r28)
	REX_STORE_U32(ctx.r28.u32 + 0, ctx.r30.u32);
	// stw r31,8(r28)
	REX_STORE_U32(ctx.r28.u32 + 8, ctx.r31.u32);
loc_82956A80:
	// addi r1,r1,128
	ctx.r1.s64 = ctx.r1.s64 + 128;
	// b 0x824131f8
	__restgprlr_28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82956A88) {
	REX_FUNC_PROLOGUE(sub_82956A88);
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a4
	ctx.lr = 0x82956A90;
	__savegprlr_27(ctx, base);
	// lwz r11,0(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);
	// lwz r10,4(r3)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r3.u32 + 4);
	// cmplw cr6,r11,r10
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
	// bne cr6,0x82956c58
	if (!ctx.cr6.eq) goto loc_82956C58;
	// lwz r10,40(r3)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r3.u32 + 40);
	// li r9,-1
	ctx.r9.s64 = -1;
	// lwz r11,8(r4)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r4.u32 + 8);
	// lwz r6,0(r4)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r4.u32 + 0);
	// lwz r7,4(r4)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r4.u32 + 4);
	// cmplw cr6,r11,r10
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
	// bge cr6,0x82956aec
	if (!ctx.cr6.lt) goto loc_82956AEC;
	// lwz r8,0(r7)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r7.u32 + 0);
	// subfic r5,r10,32
	ctx.xer.ca = ctx.r10.u32 <= 32;
	ctx.r5.u64 = static_cast<uint64_t>(32) - ctx.r10.u64;
	// subf r31,r11,r10
	ctx.r31.u64 = ctx.r10.u64 - ctx.r11.u64;
	// subf r10,r10,r11
	ctx.r10.u64 = ctx.r11.u64 - ctx.r10.u64;
	// addi r7,r7,4
	ctx.r7.s64 = ctx.r7.s64 + 4;
	// slw r30,r8,r11
	ctx.r30.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 << (ctx.r11.u8 & 0x3F));
	// addi r11,r10,32
	ctx.r11.s64 = ctx.r10.s64 + 32;
	// or r10,r30,r6
	ctx.r10.u64 = ctx.r30.u64 | ctx.r6.u64;
	// srw r5,r9,r5
	ctx.r5.u64 = ctx.r5.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r5.u8 & 0x3F));
	// and r29,r5,r10
	ctx.r29.u64 = ctx.r5.u64 & ctx.r10.u64;
	// srw r10,r8,r31
	ctx.r10.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r31.u8 & 0x3F));
	// b 0x82956b00
	goto loc_82956B00;
loc_82956AEC:
	// subfic r8,r10,32
	ctx.xer.ca = ctx.r10.u32 <= 32;
	ctx.r8.u64 = static_cast<uint64_t>(32) - ctx.r10.u64;
	// subf r11,r10,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r10.u64;
	// srw r10,r6,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r6.u32 >> (ctx.r10.u8 & 0x3F));
	// srw r8,r9,r8
	ctx.r8.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r8.u8 & 0x3F));
	// and r29,r8,r6
	ctx.r29.u64 = ctx.r8.u64 & ctx.r6.u64;
loc_82956B00:
	// cmplwi cr6,r29,0
	ctx.cr6.compare<uint32_t>(ctx.r29.u32, 0, ctx.xer);
	// beq cr6,0x82956c3c
	if (ctx.cr6.eq) goto loc_82956C3C;
	// lwz r27,32(r3)
	ctx.r27.u64 = REX_LOAD_U32(ctx.r3.u32 + 32);
	// addi r31,r3,16
	ctx.r31.s64 = ctx.r3.s64 + 16;
	// lwz r8,44(r3)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r3.u32 + 44);
	// subfic r6,r27,32
	ctx.xer.ca = ctx.r27.u32 <= 32;
	ctx.r6.u64 = static_cast<uint64_t>(32) - ctx.r27.u64;
	// lwz r5,36(r3)
	ctx.r5.u64 = REX_LOAD_U32(ctx.r3.u32 + 36);
	// add r30,r8,r29
	ctx.r30.u64 = ctx.r8.u64 + ctx.r29.u64;
	// mr r28,r8
	ctx.r28.u64 = ctx.r8.u64;
	// stw r8,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r8.u32);
	// stw r30,4(r3)
	REX_STORE_U32(ctx.r3.u32 + 4, ctx.r30.u32);
	// srw r3,r9,r6
	ctx.r3.u64 = ctx.r6.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r6.u8 & 0x3F));
loc_82956B30:
	// addi r29,r29,-1
	ctx.r29.s64 = ctx.r29.s64 + -1;
	// cmplw cr6,r11,r27
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r27.u32, ctx.xer);
	// blt cr6,0x82956b5c
	if (ctx.cr6.lt) goto loc_82956B5C;
	// and r9,r3,r10
	ctx.r9.u64 = ctx.r3.u64 & ctx.r10.u64;
	// lbzx r9,r9,r5
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + ctx.r5.u32);
	// clrlwi r8,r9,28
	ctx.r8.u64 = ctx.r9.u32 & 0xF;
	// rlwinm r9,r9,28,4,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 28) & 0xFFFFFFF;
	// subf r11,r9,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r9.u64;
	// lbzx r30,r8,r31
	ctx.r30.u64 = REX_LOAD_U8(ctx.r8.u32 + ctx.r31.u32);
	// srw r10,r10,r9
	ctx.r10.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82956ba4
	goto loc_82956BA4;
loc_82956B5C:
	// lwz r8,0(r7)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r7.u32 + 0);
	// slw r9,r8,r11
	ctx.r9.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 << (ctx.r11.u8 & 0x3F));
	// or r9,r9,r10
	ctx.r9.u64 = ctx.r9.u64 | ctx.r10.u64;
	// and r9,r9,r3
	ctx.r9.u64 = ctx.r9.u64 & ctx.r3.u64;
	// lbzx r9,r9,r5
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + ctx.r5.u32);
	// clrlwi r6,r9,28
	ctx.r6.u64 = ctx.r9.u32 & 0xF;
	// rlwinm r9,r9,28,4,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 28) & 0xFFFFFFF;
	// cmplw cr6,r11,r9
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r9.u32, ctx.xer);
	// lbzx r30,r6,r31
	ctx.r30.u64 = REX_LOAD_U8(ctx.r6.u32 + ctx.r31.u32);
	// blt cr6,0x82956b90
	if (ctx.cr6.lt) goto loc_82956B90;
	// subf r11,r9,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r9.u64;
	// srw r10,r10,r9
	ctx.r10.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82956ba4
	goto loc_82956BA4;
loc_82956B90:
	// subf r10,r11,r9
	ctx.r10.u64 = ctx.r9.u64 - ctx.r11.u64;
	// subf r11,r9,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r9.u64;
	// addi r7,r7,4
	ctx.r7.s64 = ctx.r7.s64 + 4;
	// addi r11,r11,32
	ctx.r11.s64 = ctx.r11.s64 + 32;
	// srw r10,r8,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r10.u8 & 0x3F));
loc_82956BA4:
	// cmplw cr6,r11,r27
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r27.u32, ctx.xer);
	// blt cr6,0x82956bcc
	if (ctx.cr6.lt) goto loc_82956BCC;
	// and r9,r3,r10
	ctx.r9.u64 = ctx.r3.u64 & ctx.r10.u64;
	// lbzx r9,r9,r5
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + ctx.r5.u32);
	// clrlwi r8,r9,28
	ctx.r8.u64 = ctx.r9.u32 & 0xF;
	// rlwinm r9,r9,28,4,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 28) & 0xFFFFFFF;
	// subf r11,r9,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r9.u64;
	// lbzx r6,r8,r31
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + ctx.r31.u32);
	// srw r10,r10,r9
	ctx.r10.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82956c14
	goto loc_82956C14;
loc_82956BCC:
	// lwz r8,0(r7)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r7.u32 + 0);
	// slw r9,r8,r11
	ctx.r9.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 << (ctx.r11.u8 & 0x3F));
	// or r9,r9,r10
	ctx.r9.u64 = ctx.r9.u64 | ctx.r10.u64;
	// and r9,r9,r3
	ctx.r9.u64 = ctx.r9.u64 & ctx.r3.u64;
	// lbzx r9,r9,r5
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + ctx.r5.u32);
	// clrlwi r6,r9,28
	ctx.r6.u64 = ctx.r9.u32 & 0xF;
	// rlwinm r9,r9,28,4,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 28) & 0xFFFFFFF;
	// cmplw cr6,r11,r9
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r9.u32, ctx.xer);
	// lbzx r6,r6,r31
	ctx.r6.u64 = REX_LOAD_U8(ctx.r6.u32 + ctx.r31.u32);
	// blt cr6,0x82956c00
	if (ctx.cr6.lt) goto loc_82956C00;
	// subf r11,r9,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r9.u64;
	// srw r10,r10,r9
	ctx.r10.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// b 0x82956c14
	goto loc_82956C14;
loc_82956C00:
	// subf r10,r11,r9
	ctx.r10.u64 = ctx.r9.u64 - ctx.r11.u64;
	// subf r11,r9,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r9.u64;
	// addi r7,r7,4
	ctx.r7.s64 = ctx.r7.s64 + 4;
	// addi r11,r11,32
	ctx.r11.s64 = ctx.r11.s64 + 32;
	// srw r10,r8,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r10.u8 & 0x3F));
loc_82956C14:
	// rlwinm r9,r6,4,0,27
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 4) & 0xFFFFFFF0;
	// cmplwi cr6,r29,0
	ctx.cr6.compare<uint32_t>(ctx.r29.u32, 0, ctx.xer);
	// or r9,r9,r30
	ctx.r9.u64 = ctx.r9.u64 | ctx.r30.u64;
	// stb r9,0(r28)
	REX_STORE_U8(ctx.r28.u32 + 0, ctx.r9.u8);
	// addi r28,r28,1
	ctx.r28.s64 = ctx.r28.s64 + 1;
	// bne cr6,0x82956b30
	if (!ctx.cr6.eq) goto loc_82956B30;
	// stw r7,4(r4)
	REX_STORE_U32(ctx.r4.u32 + 4, ctx.r7.u32);
	// stw r10,0(r4)
	REX_STORE_U32(ctx.r4.u32 + 0, ctx.r10.u32);
	// stw r11,8(r4)
	REX_STORE_U32(ctx.r4.u32 + 8, ctx.r11.u32);
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
loc_82956C3C:
	// lwz r9,44(r3)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r3.u32 + 44);
	// addi r8,r9,4
	ctx.r8.s64 = ctx.r9.s64 + 4;
	// stw r9,4(r3)
	REX_STORE_U32(ctx.r3.u32 + 4, ctx.r9.u32);
	// stw r8,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r8.u32);
	// stw r7,4(r4)
	REX_STORE_U32(ctx.r4.u32 + 4, ctx.r7.u32);
	// stw r10,0(r4)
	REX_STORE_U32(ctx.r4.u32 + 0, ctx.r10.u32);
	// stw r11,8(r4)
	REX_STORE_U32(ctx.r4.u32 + 8, ctx.r11.u32);
loc_82956C58:
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82956C60) {
	REX_FUNC_PROLOGUE(sub_82956C60);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a8
	ctx.lr = 0x82956C68;
	__savegprlr_28(ctx, base);
	// stwu r1,-128(r1)
	ea = -128 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r6,r3
	ctx.r6.u64 = ctx.r3.u64;
	// mr r28,r4
	ctx.r28.u64 = ctx.r4.u64;
	// lwz r11,0(r6)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// lwz r10,4(r6)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r6.u32 + 4);
	// cmplw cr6,r11,r10
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
	// bne cr6,0x82956ec8
	if (!ctx.cr6.eq) goto loc_82956EC8;
	// lwz r11,40(r6)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r6.u32 + 40);
	// li r9,-1
	ctx.r9.s64 = -1;
	// lwz r10,8(r28)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r28.u32 + 8);
	// lwz r7,0(r28)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r28.u32 + 0);
	// lwz r29,4(r28)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r28.u32 + 4);
	// cmplw cr6,r10,r11
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r11.u32, ctx.xer);
	// bge cr6,0x82956cd0
	if (!ctx.cr6.lt) goto loc_82956CD0;
	// lwz r8,0(r29)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subfic r5,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r5.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r4,r10,r11
	ctx.r4.u64 = ctx.r11.u64 - ctx.r10.u64;
	// subf r11,r11,r10
	ctx.r11.u64 = ctx.r10.u64 - ctx.r11.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// slw r10,r8,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r8.u32 << (ctx.r10.u8 & 0x3F));
	// srw r5,r9,r5
	ctx.r5.u64 = ctx.r5.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r5.u8 & 0x3F));
	// or r11,r10,r7
	ctx.r11.u64 = ctx.r10.u64 | ctx.r7.u64;
	// srw r30,r8,r4
	ctx.r30.u64 = ctx.r4.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r4.u8 & 0x3F));
	// and r5,r5,r11
	ctx.r5.u64 = ctx.r5.u64 & ctx.r11.u64;
	// b 0x82956ce4
	goto loc_82956CE4;
loc_82956CD0:
	// subfic r8,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r8.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r31,r11,r10
	ctx.r31.u64 = ctx.r10.u64 - ctx.r11.u64;
	// srw r30,r7,r11
	ctx.r30.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r7.u32 >> (ctx.r11.u8 & 0x3F));
	// srw r10,r9,r8
	ctx.r10.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r8.u8 & 0x3F));
	// and r5,r10,r7
	ctx.r5.u64 = ctx.r10.u64 & ctx.r7.u64;
loc_82956CE4:
	// cmplwi cr6,r5,0
	ctx.cr6.compare<uint32_t>(ctx.r5.u32, 0, ctx.xer);
	// beq cr6,0x82956eac
	if (ctx.cr6.eq) goto loc_82956EAC;
	// lwz r3,44(r6)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r6.u32 + 44);
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// add r11,r3,r5
	ctx.r11.u64 = ctx.r3.u64 + ctx.r5.u64;
	// stw r3,0(r6)
	REX_STORE_U32(ctx.r6.u32 + 0, ctx.r3.u32);
	// stw r11,4(r6)
	REX_STORE_U32(ctx.r6.u32 + 4, ctx.r11.u32);
	// bne cr6,0x82956d18
	if (!ctx.cr6.eq) goto loc_82956D18;
	// lwz r11,0(r29)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// li r31,31
	ctx.r31.s64 = 31;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// rlwinm r30,r11,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x82956d24
	goto loc_82956D24;
loc_82956D18:
	// mr r11,r30
	ctx.r11.u64 = ctx.r30.u64;
	// rlwinm r30,r30,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
loc_82956D24:
	// clrlwi r11,r11,31
	ctx.r11.u64 = ctx.r11.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x82956e1c
	if (!ctx.cr6.eq) goto loc_82956E1C;
	// lwz r4,32(r6)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r6.u32 + 32);
	// addi r7,r6,16
	ctx.r7.s64 = ctx.r6.s64 + 16;
	// lwz r8,36(r6)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r6.u32 + 36);
	// subfic r11,r4,32
	ctx.xer.ca = ctx.r4.u32 <= 32;
	ctx.r11.u64 = static_cast<uint64_t>(32) - ctx.r4.u64;
	// srw r6,r9,r11
	ctx.r6.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r11.u8 & 0x3F));
loc_82956D44:
	// addi r5,r5,-1
	ctx.r5.s64 = ctx.r5.s64 + -1;
	// cmplw cr6,r31,r4
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r4.u32, ctx.xer);
	// blt cr6,0x82956d70
	if (ctx.cr6.lt) goto loc_82956D70;
	// and r11,r6,r30
	ctx.r11.u64 = ctx.r6.u64 & ctx.r30.u64;
	// lbzx r11,r11,r8
	ctx.r11.u64 = REX_LOAD_U8(ctx.r11.u32 + ctx.r8.u32);
	// clrlwi r10,r11,28
	ctx.r10.u64 = ctx.r11.u32 & 0xF;
	// rlwinm r11,r11,28,4,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 28) & 0xFFFFFFF;
	// subf r31,r11,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r11.u64;
	// lbzx r9,r10,r7
	ctx.r9.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r7.u32);
	// srw r30,r30,r11
	ctx.r30.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r11.u8 & 0x3F));
	// b 0x82956db8
	goto loc_82956DB8;
loc_82956D70:
	// lwz r10,0(r29)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// slw r11,r10,r31
	ctx.r11.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r10.u32 << (ctx.r31.u8 & 0x3F));
	// or r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 | ctx.r30.u64;
	// and r11,r11,r6
	ctx.r11.u64 = ctx.r11.u64 & ctx.r6.u64;
	// lbzx r11,r11,r8
	ctx.r11.u64 = REX_LOAD_U8(ctx.r11.u32 + ctx.r8.u32);
	// clrlwi r9,r11,28
	ctx.r9.u64 = ctx.r11.u32 & 0xF;
	// rlwinm r11,r11,28,4,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 28) & 0xFFFFFFF;
	// cmplw cr6,r31,r11
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r11.u32, ctx.xer);
	// lbzx r9,r9,r7
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + ctx.r7.u32);
	// blt cr6,0x82956da4
	if (ctx.cr6.lt) goto loc_82956DA4;
	// subf r31,r11,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r11.u64;
	// srw r30,r30,r11
	ctx.r30.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r11.u8 & 0x3F));
	// b 0x82956db8
	goto loc_82956DB8;
loc_82956DA4:
	// subf r30,r31,r11
	ctx.r30.u64 = ctx.r11.u64 - ctx.r31.u64;
	// subf r11,r11,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r11.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// srw r30,r10,r30
	ctx.r30.u64 = ctx.r30.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r30.u8 & 0x3F));
loc_82956DB8:
	// cmpwi cr6,r9,0
	ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
	// beq cr6,0x82956df8
	if (ctx.cr6.eq) goto loc_82956DF8;
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// bne cr6,0x82956ddc
	if (!ctx.cr6.eq) goto loc_82956DDC;
	// lwz r11,0(r29)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// li r31,31
	ctx.r31.s64 = 31;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// rlwinm r30,r11,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x82956de8
	goto loc_82956DE8;
loc_82956DDC:
	// mr r11,r30
	ctx.r11.u64 = ctx.r30.u64;
	// rlwinm r30,r30,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
loc_82956DE8:
	// clrlwi r11,r11,31
	ctx.r11.u64 = ctx.r11.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82956df8
	if (ctx.cr6.eq) goto loc_82956DF8;
	// neg r9,r9
	ctx.r9.s64 = static_cast<int64_t>(-ctx.r9.u64);
loc_82956DF8:
	// stb r9,0(r3)
	REX_STORE_U8(ctx.r3.u32 + 0, ctx.r9.u8);
	// cmplwi cr6,r5,0
	ctx.cr6.compare<uint32_t>(ctx.r5.u32, 0, ctx.xer);
	// addi r3,r3,1
	ctx.r3.s64 = ctx.r3.s64 + 1;
	// bne cr6,0x82956d44
	if (!ctx.cr6.eq) goto loc_82956D44;
	// stw r29,4(r28)
	REX_STORE_U32(ctx.r28.u32 + 4, ctx.r29.u32);
	// stw r30,0(r28)
	REX_STORE_U32(ctx.r28.u32 + 0, ctx.r30.u32);
	// stw r31,8(r28)
	REX_STORE_U32(ctx.r28.u32 + 8, ctx.r31.u32);
	// addi r1,r1,128
	ctx.r1.s64 = ctx.r1.s64 + 128;
	// b 0x824131f8
	__restgprlr_28(ctx, base);
	return;
loc_82956E1C:
	// cmplwi cr6,r31,4
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 4, ctx.xer);
	// bge cr6,0x82956e48
	if (!ctx.cr6.lt) goto loc_82956E48;
	// lwz r11,0(r29)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subfic r10,r31,4
	ctx.xer.ca = ctx.r31.u32 <= 4;
	ctx.r10.u64 = static_cast<uint64_t>(4) - ctx.r31.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// slw r9,r11,r31
	ctx.r9.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// addi r31,r31,28
	ctx.r31.s64 = ctx.r31.s64 + 28;
	// or r9,r9,r30
	ctx.r9.u64 = ctx.r9.u64 | ctx.r30.u64;
	// srw r30,r11,r10
	ctx.r30.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r11.u32 >> (ctx.r10.u8 & 0x3F));
	// clrlwi r4,r9,28
	ctx.r4.u64 = ctx.r9.u32 & 0xF;
	// b 0x82956e54
	goto loc_82956E54;
loc_82956E48:
	// clrlwi r4,r30,28
	ctx.r4.u64 = ctx.r30.u32 & 0xF;
	// rlwinm r30,r30,28,4,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 28) & 0xFFFFFFF;
	// addi r31,r31,-4
	ctx.r31.s64 = ctx.r31.s64 + -4;
loc_82956E54:
	// cmpwi cr6,r4,0
	ctx.cr6.compare<int32_t>(ctx.r4.s32, 0, ctx.xer);
	// beq cr6,0x82956e94
	if (ctx.cr6.eq) goto loc_82956E94;
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// bne cr6,0x82956e78
	if (!ctx.cr6.eq) goto loc_82956E78;
	// lwz r11,0(r29)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// li r31,31
	ctx.r31.s64 = 31;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// rlwinm r30,r11,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x82956e84
	goto loc_82956E84;
loc_82956E78:
	// mr r11,r30
	ctx.r11.u64 = ctx.r30.u64;
	// rlwinm r30,r30,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
loc_82956E84:
	// clrlwi r11,r11,31
	ctx.r11.u64 = ctx.r11.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82956e94
	if (ctx.cr6.eq) goto loc_82956E94;
	// neg r4,r4
	ctx.r4.s64 = static_cast<int64_t>(-ctx.r4.u64);
loc_82956E94:
	// bl 0x82414530
	ctx.lr = 0x82956E98;
	sub_82414530(ctx, base);
	// stw r29,4(r28)
	REX_STORE_U32(ctx.r28.u32 + 4, ctx.r29.u32);
	// stw r30,0(r28)
	REX_STORE_U32(ctx.r28.u32 + 0, ctx.r30.u32);
	// stw r31,8(r28)
	REX_STORE_U32(ctx.r28.u32 + 8, ctx.r31.u32);
	// addi r1,r1,128
	ctx.r1.s64 = ctx.r1.s64 + 128;
	// b 0x824131f8
	__restgprlr_28(ctx, base);
	return;
loc_82956EAC:
	// lwz r11,44(r6)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r6.u32 + 44);
	// addi r10,r11,4
	ctx.r10.s64 = ctx.r11.s64 + 4;
	// stw r11,4(r6)
	REX_STORE_U32(ctx.r6.u32 + 4, ctx.r11.u32);
	// stw r10,0(r6)
	REX_STORE_U32(ctx.r6.u32 + 0, ctx.r10.u32);
	// stw r29,4(r28)
	REX_STORE_U32(ctx.r28.u32 + 4, ctx.r29.u32);
	// stw r30,0(r28)
	REX_STORE_U32(ctx.r28.u32 + 0, ctx.r30.u32);
	// stw r31,8(r28)
	REX_STORE_U32(ctx.r28.u32 + 8, ctx.r31.u32);
loc_82956EC8:
	// addi r1,r1,128
	ctx.r1.s64 = ctx.r1.s64 + 128;
	// b 0x824131f8
	__restgprlr_28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82956ED0) {
	REX_FUNC_PROLOGUE(sub_82956ED0);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x8241318c
	ctx.lr = 0x82956ED8;
	__savegprlr_21(ctx, base);
	// stwu r1,-176(r1)
	ea = -176 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lwz r11,0(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);
	// mr r21,r4
	ctx.r21.u64 = ctx.r4.u64;
	// lwz r10,4(r3)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r3.u32 + 4);
	// cmplw cr6,r11,r10
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
	// bne cr6,0x829571d4
	if (!ctx.cr6.eq) goto loc_829571D4;
	// lwz r11,40(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 40);
	// li r24,-1
	ctx.r24.s64 = -1;
	// lwz r10,8(r21)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r21.u32 + 8);
	// lwz r8,0(r21)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r21.u32 + 0);
	// lwz r29,4(r21)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r21.u32 + 4);
	// cmplw cr6,r10,r11
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r11.u32, ctx.xer);
	// bge cr6,0x82956f3c
	if (!ctx.cr6.lt) goto loc_82956F3C;
	// lwz r9,0(r29)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subfic r7,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r7.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r6,r10,r11
	ctx.r6.u64 = ctx.r11.u64 - ctx.r10.u64;
	// subf r11,r11,r10
	ctx.r11.u64 = ctx.r10.u64 - ctx.r11.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// slw r10,r9,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r10.u8 & 0x3F));
	// srw r7,r24,r7
	ctx.r7.u64 = ctx.r7.u8 & 0x20 ? 0 : (ctx.r24.u32 >> (ctx.r7.u8 & 0x3F));
	// or r11,r10,r8
	ctx.r11.u64 = ctx.r10.u64 | ctx.r8.u64;
	// srw r30,r9,r6
	ctx.r30.u64 = ctx.r6.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r6.u8 & 0x3F));
	// and r7,r7,r11
	ctx.r7.u64 = ctx.r7.u64 & ctx.r11.u64;
	// b 0x82956f50
	goto loc_82956F50;
loc_82956F3C:
	// subfic r9,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r9.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r31,r11,r10
	ctx.r31.u64 = ctx.r10.u64 - ctx.r11.u64;
	// srw r30,r8,r11
	ctx.r30.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r11.u8 & 0x3F));
	// srw r10,r24,r9
	ctx.r10.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r24.u32 >> (ctx.r9.u8 & 0x3F));
	// and r7,r10,r8
	ctx.r7.u64 = ctx.r10.u64 & ctx.r8.u64;
loc_82956F50:
	// cmplwi cr6,r7,0
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 0, ctx.xer);
	// beq cr6,0x829571b8
	if (ctx.cr6.eq) goto loc_829571B8;
	// lwz r11,12(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 12);
	// lwz r8,44(r3)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r3.u32 + 44);
	// cmpwi cr6,r11,0
	ctx.cr6.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
	// lwz r11,8(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 8);
	// beq cr6,0x82957008
	if (ctx.cr6.eq) goto loc_82957008;
	// addi r10,r11,-1
	ctx.r10.s64 = ctx.r11.s64 + -1;
	// cmplw cr6,r31,r10
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r10.u32, ctx.xer);
	// bge cr6,0x82956fac
	if (!ctx.cr6.lt) goto loc_82956FAC;
	// lwz r10,0(r29)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subf r9,r31,r11
	ctx.r9.u64 = ctx.r11.u64 - ctx.r31.u64;
	// subfic r6,r11,33
	ctx.xer.ca = ctx.r11.u32 <= 33;
	ctx.r6.u64 = static_cast<uint64_t>(33) - ctx.r11.u64;
	// addi r9,r9,-1
	ctx.r9.s64 = ctx.r9.s64 + -1;
	// subf r11,r11,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r11.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// slw r5,r10,r31
	ctx.r5.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r10.u32 << (ctx.r31.u8 & 0x3F));
	// addi r31,r11,33
	ctx.r31.s64 = ctx.r11.s64 + 33;
	// or r11,r5,r30
	ctx.r11.u64 = ctx.r5.u64 | ctx.r30.u64;
	// srw r6,r24,r6
	ctx.r6.u64 = ctx.r6.u8 & 0x20 ? 0 : (ctx.r24.u32 >> (ctx.r6.u8 & 0x3F));
	// srw r30,r10,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r9.u8 & 0x3F));
	// and r25,r6,r11
	ctx.r25.u64 = ctx.r6.u64 & ctx.r11.u64;
	// b 0x82956fc4
	goto loc_82956FC4;
loc_82956FAC:
	// subfic r9,r11,33
	ctx.xer.ca = ctx.r11.u32 <= 33;
	ctx.r9.u64 = static_cast<uint64_t>(33) - ctx.r11.u64;
	// subf r11,r11,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r11.u64;
	// addi r31,r11,1
	ctx.r31.s64 = ctx.r11.s64 + 1;
	// srw r11,r24,r9
	ctx.r11.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r24.u32 >> (ctx.r9.u8 & 0x3F));
	// and r25,r11,r30
	ctx.r25.u64 = ctx.r11.u64 & ctx.r30.u64;
	// srw r30,r30,r10
	ctx.r30.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r10.u8 & 0x3F));
loc_82956FC4:
	// cmpwi cr6,r25,0
	ctx.cr6.compare<int32_t>(ctx.r25.s32, 0, ctx.xer);
	// beq cr6,0x82957054
	if (ctx.cr6.eq) goto loc_82957054;
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// bne cr6,0x82956fe8
	if (!ctx.cr6.eq) goto loc_82956FE8;
	// lwz r11,0(r29)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// li r31,31
	ctx.r31.s64 = 31;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// rlwinm r30,r11,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x82956ff4
	goto loc_82956FF4;
loc_82956FE8:
	// mr r11,r30
	ctx.r11.u64 = ctx.r30.u64;
	// rlwinm r30,r30,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
loc_82956FF4:
	// clrlwi r11,r11,31
	ctx.r11.u64 = ctx.r11.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82957054
	if (ctx.cr6.eq) goto loc_82957054;
	// neg r25,r25
	ctx.r25.s64 = static_cast<int64_t>(-ctx.r25.u64);
	// b 0x82957054
	goto loc_82957054;
loc_82957008:
	// cmplw cr6,r31,r11
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r11.u32, ctx.xer);
	// bge cr6,0x82957040
	if (!ctx.cr6.lt) goto loc_82957040;
	// lwz r10,0(r29)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subfic r9,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r9.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r6,r31,r11
	ctx.r6.u64 = ctx.r11.u64 - ctx.r31.u64;
	// subf r11,r11,r31
	ctx.r11.u64 = ctx.r31.u64 - ctx.r11.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// slw r5,r10,r31
	ctx.r5.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r10.u32 << (ctx.r31.u8 & 0x3F));
	// addi r31,r11,32
	ctx.r31.s64 = ctx.r11.s64 + 32;
	// or r11,r5,r30
	ctx.r11.u64 = ctx.r5.u64 | ctx.r30.u64;
	// srw r9,r24,r9
	ctx.r9.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r24.u32 >> (ctx.r9.u8 & 0x3F));
	// srw r30,r10,r6
	ctx.r30.u64 = ctx.r6.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r6.u8 & 0x3F));
	// and r25,r9,r11
	ctx.r25.u64 = ctx.r9.u64 & ctx.r11.u64;
	// b 0x82957054
	goto loc_82957054;
loc_82957040:
	// subfic r10,r11,32
	ctx.xer.ca = ctx.r11.u32 <= 32;
	ctx.r10.u64 = static_cast<uint64_t>(32) - ctx.r11.u64;
	// subf r31,r11,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r11.u64;
	// srw r10,r24,r10
	ctx.r10.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r24.u32 >> (ctx.r10.u8 & 0x3F));
	// and r25,r10,r30
	ctx.r25.u64 = ctx.r10.u64 & ctx.r30.u64;
	// srw r30,r30,r11
	ctx.r30.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r11.u8 & 0x3F));
loc_82957054:
	// clrlwi r28,r25,16
	ctx.r28.u64 = ctx.r25.u32 & 0xFFFF;
	// rlwinm r10,r7,1,0,30
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 1) & 0xFFFFFFFE;
	// addi r22,r7,-1
	ctx.r22.s64 = ctx.r7.s64 + -1;
	// addi r26,r8,2
	ctx.r26.s64 = ctx.r8.s64 + 2;
	// cmplwi cr6,r22,0
	ctx.cr6.compare<uint32_t>(ctx.r22.u32, 0, ctx.xer);
	// sth r28,0(r8)
	REX_STORE_U16(ctx.r8.u32 + 0, ctx.r28.u16);
	// lwz r11,44(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 44);
	// add r10,r10,r11
	ctx.r10.u64 = ctx.r10.u64 + ctx.r11.u64;
	// stw r11,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r11.u32);
	// stw r10,4(r3)
	REX_STORE_U32(ctx.r3.u32 + 4, ctx.r10.u32);
	// beq cr6,0x829571c8
	if (ctx.cr6.eq) goto loc_829571C8;
loc_82957080:
	// cmplwi cr6,r22,8
	ctx.cr6.compare<uint32_t>(ctx.r22.u32, 8, ctx.xer);
	// li r23,8
	ctx.r23.s64 = 8;
	// bgt cr6,0x82957090
	if (ctx.cr6.gt) goto loc_82957090;
	// mr r23,r22
	ctx.r23.u64 = ctx.r22.u64;
loc_82957090:
	// cmplwi cr6,r31,4
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 4, ctx.xer);
	// bge cr6,0x829570bc
	if (!ctx.cr6.lt) goto loc_829570BC;
	// lwz r11,0(r29)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subfic r9,r31,4
	ctx.xer.ca = ctx.r31.u32 <= 4;
	ctx.r9.u64 = static_cast<uint64_t>(4) - ctx.r31.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// slw r10,r11,r31
	ctx.r10.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// addi r31,r31,28
	ctx.r31.s64 = ctx.r31.s64 + 28;
	// or r10,r10,r30
	ctx.r10.u64 = ctx.r10.u64 | ctx.r30.u64;
	// srw r30,r11,r9
	ctx.r30.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r11.u32 >> (ctx.r9.u8 & 0x3F));
	// clrlwi r10,r10,28
	ctx.r10.u64 = ctx.r10.u32 & 0xF;
	// b 0x829570c8
	goto loc_829570C8;
loc_829570BC:
	// clrlwi r10,r30,28
	ctx.r10.u64 = ctx.r30.u32 & 0xF;
	// rlwinm r30,r30,28,4,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 28) & 0xFFFFFFF;
	// addi r31,r31,-4
	ctx.r31.s64 = ctx.r31.s64 + -4;
loc_829570C8:
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x82957180
	if (ctx.cr6.eq) goto loc_82957180;
	// mr r7,r23
	ctx.r7.u64 = ctx.r23.u64;
	// cmpwi cr6,r23,0
	ctx.cr6.compare<int32_t>(ctx.r23.s32, 0, ctx.xer);
	// beq cr6,0x82957198
	if (ctx.cr6.eq) goto loc_82957198;
	// subfic r11,r10,32
	ctx.xer.ca = ctx.r10.u32 <= 32;
	ctx.r11.u64 = static_cast<uint64_t>(32) - ctx.r10.u64;
	// srw r8,r24,r11
	ctx.r8.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r24.u32 >> (ctx.r11.u8 & 0x3F));
loc_829570E4:
	// addi r7,r7,-1
	ctx.r7.s64 = ctx.r7.s64 + -1;
	// cmplw cr6,r31,r10
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r10.u32, ctx.xer);
	// bge cr6,0x82957118
	if (!ctx.cr6.lt) goto loc_82957118;
	// lwz r11,0(r29)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// subf r6,r31,r10
	ctx.r6.u64 = ctx.r10.u64 - ctx.r31.u64;
	// subf r9,r10,r31
	ctx.r9.u64 = ctx.r31.u64 - ctx.r10.u64;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// slw r5,r11,r31
	ctx.r5.u64 = ctx.r31.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r31.u8 & 0x3F));
	// addi r31,r9,32
	ctx.r31.s64 = ctx.r9.s64 + 32;
	// or r9,r5,r30
	ctx.r9.u64 = ctx.r5.u64 | ctx.r30.u64;
	// srw r30,r11,r6
	ctx.r30.u64 = ctx.r6.u8 & 0x20 ? 0 : (ctx.r11.u32 >> (ctx.r6.u8 & 0x3F));
	// and r9,r9,r8
	ctx.r9.u64 = ctx.r9.u64 & ctx.r8.u64;
	// b 0x82957124
	goto loc_82957124;
loc_82957118:
	// and r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 & ctx.r30.u64;
	// subf r31,r10,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r10.u64;
	// srw r30,r30,r10
	ctx.r30.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r30.u32 >> (ctx.r10.u8 & 0x3F));
loc_82957124:
	// cmpwi cr6,r9,0
	ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
	// beq cr6,0x82957164
	if (ctx.cr6.eq) goto loc_82957164;
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// bne cr6,0x82957148
	if (!ctx.cr6.eq) goto loc_82957148;
	// lwz r11,0(r29)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);
	// li r31,31
	ctx.r31.s64 = 31;
	// addi r29,r29,4
	ctx.r29.s64 = ctx.r29.s64 + 4;
	// rlwinm r30,r11,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x82957154
	goto loc_82957154;
loc_82957148:
	// mr r11,r30
	ctx.r11.u64 = ctx.r30.u64;
	// rlwinm r30,r30,31,1,31
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
loc_82957154:
	// clrlwi r11,r11,31
	ctx.r11.u64 = ctx.r11.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82957164
	if (ctx.cr6.eq) goto loc_82957164;
	// neg r9,r9
	ctx.r9.s64 = static_cast<int64_t>(-ctx.r9.u64);
loc_82957164:
	// add r25,r9,r25
	ctx.r25.u64 = ctx.r9.u64 + ctx.r25.u64;
	// cmpwi cr6,r7,0
	ctx.cr6.compare<int32_t>(ctx.r7.s32, 0, ctx.xer);
	// clrlwi r28,r25,16
	ctx.r28.u64 = ctx.r25.u32 & 0xFFFF;
	// sth r28,0(r26)
	REX_STORE_U16(ctx.r26.u32 + 0, ctx.r28.u16);
	// addi r26,r26,2
	ctx.r26.s64 = ctx.r26.s64 + 2;
	// bne cr6,0x829570e4
	if (!ctx.cr6.eq) goto loc_829570E4;
	// b 0x82957198
	goto loc_82957198;
loc_82957180:
	// rlwinm r27,r23,1,0,30
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 1) & 0xFFFFFFFE;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// mr r3,r26
	ctx.r3.u64 = ctx.r26.u64;
	// mr r5,r27
	ctx.r5.u64 = ctx.r27.u64;
	// bl 0x8295e750
	ctx.lr = 0x82957194;
	sub_8295E750(ctx, base);
	// add r26,r27,r26
	ctx.r26.u64 = ctx.r27.u64 + ctx.r26.u64;
loc_82957198:
	// subf r22,r23,r22
	ctx.r22.u64 = ctx.r22.u64 - ctx.r23.u64;
	// cmplwi cr6,r22,0
	ctx.cr6.compare<uint32_t>(ctx.r22.u32, 0, ctx.xer);
	// bne cr6,0x82957080
	if (!ctx.cr6.eq) goto loc_82957080;
	// stw r29,4(r21)
	REX_STORE_U32(ctx.r21.u32 + 4, ctx.r29.u32);
	// stw r30,0(r21)
	REX_STORE_U32(ctx.r21.u32 + 0, ctx.r30.u32);
	// stw r31,8(r21)
	REX_STORE_U32(ctx.r21.u32 + 8, ctx.r31.u32);
	// addi r1,r1,176
	ctx.r1.s64 = ctx.r1.s64 + 176;
	// b 0x824131dc
	__restgprlr_21(ctx, base);
	return;
loc_829571B8:
	// lwz r11,44(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 44);
	// addi r10,r11,4
	ctx.r10.s64 = ctx.r11.s64 + 4;
	// stw r11,4(r3)
	REX_STORE_U32(ctx.r3.u32 + 4, ctx.r11.u32);
	// stw r10,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r10.u32);
loc_829571C8:
	// stw r29,4(r21)
	REX_STORE_U32(ctx.r21.u32 + 4, ctx.r29.u32);
	// stw r30,0(r21)
	REX_STORE_U32(ctx.r21.u32 + 0, ctx.r30.u32);
	// stw r31,8(r21)
	REX_STORE_U32(ctx.r21.u32 + 8, ctx.r31.u32);
loc_829571D4:
	// addi r1,r1,176
	ctx.r1.s64 = ctx.r1.s64 + 176;
	// b 0x824131dc
	__restgprlr_21(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_829571E0) {
	REX_FUNC_PROLOGUE(sub_829571E0);
	// rlwinm r11,r4,28,4,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r4.u32 | (ctx.r4.u64 << 32), 28) & 0xFFFFFFF;
	// rlwinm r10,r4,29,3,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r4.u32 | (ctx.r4.u64 << 32), 29) & 0x1FFFFFFF;
	// addi r11,r11,515
	ctx.r11.s64 = ctx.r11.s64 + 515;
	// rlwinm r11,r11,0,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 0) & 0xFFFFFFFC;
	// stw r11,4(r3)
	REX_STORE_U32(ctx.r3.u32 + 4, ctx.r11.u32);
	// rlwinm r11,r10,3,0,28
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 3) & 0xFFFFFFF8;
	// rlwinm r10,r10,1,3,30
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 1) & 0x1FFFFFFE;
	// rlwinm r8,r11,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// addi r10,r10,1027
	ctx.r10.s64 = ctx.r10.s64 + 1027;
	// add r6,r11,r8
	ctx.r6.u64 = ctx.r11.u64 + ctx.r8.u64;
	// rlwinm r9,r10,0,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 0) & 0xFFFFFFFC;
	// rlwinm r10,r11,29,3,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 29) & 0x1FFFFFFF;
	// rlwinm r7,r11,3,3,28
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 3) & 0x1FFFFFF8;
	// clrlwi r8,r11,3
	ctx.r8.u64 = ctx.r11.u32 & 0x1FFFFFFF;
	// rlwinm r11,r6,1,3,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 1) & 0x1FFFFFFE;
	// addi r10,r10,515
	ctx.r10.s64 = ctx.r10.s64 + 515;
	// stw r9,24(r3)
	REX_STORE_U32(ctx.r3.u32 + 24, ctx.r9.u32);
	// addi r7,r7,515
	ctx.r7.s64 = ctx.r7.s64 + 515;
	// stw r9,28(r3)
	REX_STORE_U32(ctx.r3.u32 + 28, ctx.r9.u32);
	// addi r8,r8,515
	ctx.r8.s64 = ctx.r8.s64 + 515;
	// addi r11,r11,515
	ctx.r11.s64 = ctx.r11.s64 + 515;
	// rlwinm r10,r10,0,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 0) & 0xFFFFFFFC;
	// rlwinm r7,r7,0,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 0) & 0xFFFFFFFC;
	// rlwinm r8,r8,0,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 0) & 0xFFFFFFFC;
	// rlwinm r11,r11,0,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 0) & 0xFFFFFFFC;
	// stw r10,0(r3)
	REX_STORE_U32(ctx.r3.u32 + 0, ctx.r10.u32);
	// stw r7,8(r3)
	REX_STORE_U32(ctx.r3.u32 + 8, ctx.r7.u32);
	// stw r8,12(r3)
	REX_STORE_U32(ctx.r3.u32 + 12, ctx.r8.u32);
	// stw r11,32(r3)
	REX_STORE_U32(ctx.r3.u32 + 32, ctx.r11.u32);
	// stw r10,16(r3)
	REX_STORE_U32(ctx.r3.u32 + 16, ctx.r10.u32);
	// stw r10,20(r3)
	REX_STORE_U32(ctx.r3.u32 + 20, ctx.r10.u32);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82957260) {
	REX_FUNC_PROLOGUE(sub_82957260);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413170
	ctx.lr = 0x82957268;
	__savegprlr_14(ctx, base);
	// stwu r1,-1312(r1)
	ea = -1312 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r30,r7
	ctx.r30.u64 = ctx.r7.u64;
	// lwz r9,1420(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 1420);
	// li r18,0
	ctx.r18.s64 = 0;
	// stw r8,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r8.u32);
	// mr r17,r5
	ctx.r17.u64 = ctx.r5.u64;
	// stw r8,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r8.u32);
	// rlwinm r11,r30,3,0,28
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 3) & 0xFFFFFFF8;
	// mr r25,r6
	ctx.r25.u64 = ctx.r6.u64;
	// mr r20,r4
	ctx.r20.u64 = ctx.r4.u64;
	// mr r23,r10
	ctx.r23.u64 = ctx.r10.u64;
	// stw r18,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r18.u32);
	// subf r27,r17,r11
	ctx.r27.u64 = ctx.r11.u64 - ctx.r17.u64;
	// stw r17,1348(r1)
	REX_STORE_U32(ctx.r1.u32 + 1348, ctx.r17.u32);
	// mr r24,r18
	ctx.r24.u64 = ctx.r18.u64;
	// stw r18,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r18.u32);
	// rlwinm r9,r9,0,16,16
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 0) & 0x8000;
	// stw r25,1356(r1)
	REX_STORE_U32(ctx.r1.u32 + 1356, ctx.r25.u32);
	// mr r26,r3
	ctx.r26.u64 = ctx.r3.u64;
	// stw r20,96(r1)
	REX_STORE_U32(ctx.r1.u32 + 96, ctx.r20.u32);
	// stw r23,112(r1)
	REX_STORE_U32(ctx.r1.u32 + 112, ctx.r23.u32);
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// stw r27,176(r1)
	REX_STORE_U32(ctx.r1.u32 + 176, ctx.r27.u32);
	// stw r24,108(r1)
	REX_STORE_U32(ctx.r1.u32 + 108, ctx.r24.u32);
	// stw r18,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r18.u32);
	// beq cr6,0x829572dc
	if (ctx.cr6.eq) goto loc_829572DC;
	// lis r11,-32107
	ctx.r11.s64 = -2104164352;
	// addi r11,r11,25472
	ctx.r11.s64 = ctx.r11.s64 + 25472;
	// b 0x829572e4
	goto loc_829572E4;
loc_829572DC:
	// lis r11,-32107
	ctx.r11.s64 = -2104164352;
	// addi r11,r11,26120
	ctx.r11.s64 = ctx.r11.s64 + 26120;
loc_829572E4:
	// stw r11,120(r1)
	REX_STORE_U32(ctx.r1.u32 + 120, ctx.r11.u32);
	// rlwinm r10,r17,29,3,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r17.u32 | (ctx.r17.u64 << 32), 29) & 0x1FFFFFFF;
	// lwz r11,1412(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 1412);
	// li r29,16
	ctx.r29.s64 = 16;
	// stw r18,480(r1)
	REX_STORE_U32(ctx.r1.u32 + 480, ctx.r18.u32);
	// addi r6,r1,496
	ctx.r6.s64 = ctx.r1.s64 + 496;
	// stw r18,484(r1)
	REX_STORE_U32(ctx.r1.u32 + 484, ctx.r18.u32);
	// addi r5,r1,512
	ctx.r5.s64 = ctx.r1.s64 + 512;
	// stw r18,492(r1)
	REX_STORE_U32(ctx.r1.u32 + 492, ctx.r18.u32);
	// addi r4,r1,516
	ctx.r4.s64 = ctx.r1.s64 + 516;
	// stw r18,336(r1)
	REX_STORE_U32(ctx.r1.u32 + 336, ctx.r18.u32);
	// lwz r8,4(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 4);
	// lwz r9,0(r11)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// stw r18,340(r1)
	REX_STORE_U32(ctx.r1.u32 + 340, ctx.r18.u32);
	// stw r18,348(r1)
	REX_STORE_U32(ctx.r1.u32 + 348, ctx.r18.u32);
	// stw r18,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r18.u32);
	// stw r8,380(r1)
	REX_STORE_U32(ctx.r1.u32 + 380, ctx.r8.u32);
	// lwz r8,8(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 8);
	// stw r9,524(r1)
	REX_STORE_U32(ctx.r1.u32 + 524, ctx.r9.u32);
	// rlwinm r9,r17,28,4,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r17.u32 | (ctx.r17.u64 << 32), 28) & 0xFFFFFFF;
	// stw r18,132(r1)
	REX_STORE_U32(ctx.r1.u32 + 132, ctx.r18.u32);
	// addi r9,r9,511
	ctx.r9.s64 = ctx.r9.s64 + 511;
	// stw r18,140(r1)
	REX_STORE_U32(ctx.r1.u32 + 140, ctx.r18.u32);
	// stw r18,432(r1)
	REX_STORE_U32(ctx.r1.u32 + 432, ctx.r18.u32);
	// stw r8,172(r1)
	REX_STORE_U32(ctx.r1.u32 + 172, ctx.r8.u32);
	// lwz r8,12(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 12);
	// stw r18,436(r1)
	REX_STORE_U32(ctx.r1.u32 + 436, ctx.r18.u32);
	// stw r18,444(r1)
	REX_STORE_U32(ctx.r1.u32 + 444, ctx.r18.u32);
	// stw r18,240(r1)
	REX_STORE_U32(ctx.r1.u32 + 240, ctx.r18.u32);
	// stw r18,244(r1)
	REX_STORE_U32(ctx.r1.u32 + 244, ctx.r18.u32);
	// stw r8,476(r1)
	REX_STORE_U32(ctx.r1.u32 + 476, ctx.r8.u32);
	// lwz r8,16(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 16);
	// stw r29,252(r1)
	REX_STORE_U32(ctx.r1.u32 + 252, ctx.r29.u32);
	// stw r18,192(r1)
	REX_STORE_U32(ctx.r1.u32 + 192, ctx.r18.u32);
	// stw r18,196(r1)
	REX_STORE_U32(ctx.r1.u32 + 196, ctx.r18.u32);
	// stw r29,204(r1)
	REX_STORE_U32(ctx.r1.u32 + 204, ctx.r29.u32);
	// stw r8,284(r1)
	REX_STORE_U32(ctx.r1.u32 + 284, ctx.r8.u32);
	// clrlwi r8,r9,16
	ctx.r8.u64 = ctx.r9.u32 & 0xFFFF;
	// lwz r9,20(r11)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r11.u32 + 20);
	// cntlzw r7,r8
	ctx.r7.u64 = ctx.r8.u32 == 0 ? 32 : __builtin_clz(ctx.r8.u32);
	// lwz r8,28(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 28);
	// stw r18,384(r1)
	REX_STORE_U32(ctx.r1.u32 + 384, ctx.r18.u32);
	// stw r18,388(r1)
	REX_STORE_U32(ctx.r1.u32 + 388, ctx.r18.u32);
	// stw r18,396(r1)
	REX_STORE_U32(ctx.r1.u32 + 396, ctx.r18.u32);
	// stw r9,236(r1)
	REX_STORE_U32(ctx.r1.u32 + 236, ctx.r9.u32);
	// lwz r9,24(r11)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r11.u32 + 24);
	// lwz r11,32(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 32);
	// stw r8,636(r1)
	REX_STORE_U32(ctx.r1.u32 + 636, ctx.r8.u32);
	// rlwinm r8,r10,6,0,25
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 6) & 0xFFFFFFC0;
	// stw r18,592(r1)
	REX_STORE_U32(ctx.r1.u32 + 592, ctx.r18.u32);
	// stw r18,596(r1)
	REX_STORE_U32(ctx.r1.u32 + 596, ctx.r18.u32);
	// addi r8,r8,511
	ctx.r8.s64 = ctx.r8.s64 + 511;
	// stw r9,428(r1)
	REX_STORE_U32(ctx.r1.u32 + 428, ctx.r9.u32);
	// li r9,4
	ctx.r9.s64 = 4;
	// stw r11,332(r1)
	REX_STORE_U32(ctx.r1.u32 + 332, ctx.r11.u32);
	// li r11,8
	ctx.r11.s64 = 8;
	// stw r18,288(r1)
	REX_STORE_U32(ctx.r1.u32 + 288, ctx.r18.u32);
	// stw r18,292(r1)
	REX_STORE_U32(ctx.r1.u32 + 292, ctx.r18.u32);
	// stw r18,300(r1)
	REX_STORE_U32(ctx.r1.u32 + 300, ctx.r18.u32);
	// stw r9,488(r1)
	REX_STORE_U32(ctx.r1.u32 + 488, ctx.r9.u32);
	// stw r11,136(r1)
	REX_STORE_U32(ctx.r1.u32 + 136, ctx.r11.u32);
	// stw r11,440(r1)
	REX_STORE_U32(ctx.r1.u32 + 440, ctx.r11.u32);
	// li r11,5
	ctx.r11.s64 = 5;
	// stw r9,344(r1)
	REX_STORE_U32(ctx.r1.u32 + 344, ctx.r9.u32);
	// stw r9,296(r1)
	REX_STORE_U32(ctx.r1.u32 + 296, ctx.r9.u32);
	// stw r11,248(r1)
	REX_STORE_U32(ctx.r1.u32 + 248, ctx.r11.u32);
	// stw r11,200(r1)
	REX_STORE_U32(ctx.r1.u32 + 200, ctx.r11.u32);
	// li r11,11
	ctx.r11.s64 = 11;
	// stw r11,392(r1)
	REX_STORE_U32(ctx.r1.u32 + 392, ctx.r11.u32);
	// stw r11,600(r1)
	REX_STORE_U32(ctx.r1.u32 + 600, ctx.r11.u32);
	// li r11,1024
	ctx.r11.s64 = 1024;
	// stw r11,604(r1)
	REX_STORE_U32(ctx.r1.u32 + 604, ctx.r11.u32);
	// addi r11,r10,511
	ctx.r11.s64 = ctx.r10.s64 + 511;
	// clrlwi r9,r11,16
	ctx.r9.u64 = ctx.r11.u32 & 0xFFFF;
	// rlwinm r11,r10,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 1) & 0xFFFFFFFE;
	// cntlzw r9,r9
	ctx.r9.u64 = ctx.r9.u32 == 0 ? 32 : __builtin_clz(ctx.r9.u32);
	// add r3,r10,r11
	ctx.r3.u64 = ctx.r10.u64 + ctx.r11.u64;
	// subfic r11,r9,32
	ctx.xer.ca = ctx.r9.u32 <= 32;
	ctx.r11.u64 = static_cast<uint64_t>(32) - ctx.r9.u64;
	// rlwinm r9,r10,3,0,28
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 3) & 0xFFFFFFF8;
	// rlwinm r10,r3,4,0,27
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 4) & 0xFFFFFFF0;
	// addi r9,r9,511
	ctx.r9.s64 = ctx.r9.s64 + 511;
	// addi r10,r10,511
	ctx.r10.s64 = ctx.r10.s64 + 511;
	// stw r11,520(r1)
	REX_STORE_U32(ctx.r1.u32 + 520, ctx.r11.u32);
	// clrlwi r8,r8,16
	ctx.r8.u64 = ctx.r8.u32 & 0xFFFF;
	// stw r11,280(r1)
	REX_STORE_U32(ctx.r1.u32 + 280, ctx.r11.u32);
	// clrlwi r9,r9,16
	ctx.r9.u64 = ctx.r9.u32 & 0xFFFF;
	// stw r11,232(r1)
	REX_STORE_U32(ctx.r1.u32 + 232, ctx.r11.u32);
	// clrlwi r10,r10,16
	ctx.r10.u64 = ctx.r10.u32 & 0xFFFF;
	// stw r11,424(r1)
	REX_STORE_U32(ctx.r1.u32 + 424, ctx.r11.u32);
	// cntlzw r8,r8
	ctx.r8.u64 = ctx.r8.u32 == 0 ? 32 : __builtin_clz(ctx.r8.u32);
	// stw r11,632(r1)
	REX_STORE_U32(ctx.r1.u32 + 632, ctx.r11.u32);
	// cntlzw r9,r9
	ctx.r9.u64 = ctx.r9.u32 == 0 ? 32 : __builtin_clz(ctx.r9.u32);
	// subfic r7,r7,32
	ctx.xer.ca = ctx.r7.u32 <= 32;
	ctx.r7.u64 = static_cast<uint64_t>(32) - ctx.r7.u64;
	// cntlzw r10,r10
	ctx.r10.u64 = ctx.r10.u32 == 0 ? 32 : __builtin_clz(ctx.r10.u32);
	// subfic r8,r8,32
	ctx.xer.ca = ctx.r8.u32 <= 32;
	ctx.r8.u64 = static_cast<uint64_t>(32) - ctx.r8.u64;
	// subfic r9,r9,32
	ctx.xer.ca = ctx.r9.u32 <= 32;
	ctx.r9.u64 = static_cast<uint64_t>(32) - ctx.r9.u64;
	// subfic r10,r10,32
	ctx.xer.ca = ctx.r10.u32 <= 32;
	ctx.r10.u64 = static_cast<uint64_t>(32) - ctx.r10.u64;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// stw r7,376(r1)
	REX_STORE_U32(ctx.r1.u32 + 376, ctx.r7.u32);
	// stw r8,168(r1)
	REX_STORE_U32(ctx.r1.u32 + 168, ctx.r8.u32);
	// stw r9,472(r1)
	REX_STORE_U32(ctx.r1.u32 + 472, ctx.r9.u32);
	// stw r10,328(r1)
	REX_STORE_U32(ctx.r1.u32 + 328, ctx.r10.u32);
	// bl 0x829558a0
	ctx.lr = 0x82957480;
	sub_829558A0(ctx, base);
	// addi r6,r1,352
	ctx.r6.s64 = ctx.r1.s64 + 352;
	// addi r5,r1,368
	ctx.r5.s64 = ctx.r1.s64 + 368;
	// addi r4,r1,372
	ctx.r4.s64 = ctx.r1.s64 + 372;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x829558a0
	ctx.lr = 0x82957494;
	sub_829558A0(ctx, base);
	// addi r31,r1,1088
	ctx.r31.s64 = ctx.r1.s64 + 1088;
	// addi r28,r1,768
	ctx.r28.s64 = ctx.r1.s64 + 768;
loc_8295749C:
	// mr r6,r28
	ctx.r6.u64 = ctx.r28.u64;
	// addi r5,r31,-64
	ctx.r5.s64 = ctx.r31.s64 + -64;
	// mr r4,r31
	ctx.r4.u64 = ctx.r31.u64;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x829558a0
	ctx.lr = 0x829574B0;
	sub_829558A0(ctx, base);
	// addi r29,r29,-1
	ctx.r29.s64 = ctx.r29.s64 + -1;
	// addi r28,r28,16
	ctx.r28.s64 = ctx.r28.s64 + 16;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// cmplwi cr6,r29,0
	ctx.cr6.compare<uint32_t>(ctx.r29.u32, 0, ctx.xer);
	// bne cr6,0x8295749c
	if (!ctx.cr6.eq) goto loc_8295749C;
	// addi r6,r1,144
	ctx.r6.s64 = ctx.r1.s64 + 144;
	// addi r5,r1,160
	ctx.r5.s64 = ctx.r1.s64 + 160;
	// addi r4,r1,164
	ctx.r4.s64 = ctx.r1.s64 + 164;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x829558a0
	ctx.lr = 0x829574D8;
	sub_829558A0(ctx, base);
	// addi r6,r1,448
	ctx.r6.s64 = ctx.r1.s64 + 448;
	// addi r5,r1,464
	ctx.r5.s64 = ctx.r1.s64 + 464;
	// stw r18,1152(r1)
	REX_STORE_U32(ctx.r1.u32 + 1152, ctx.r18.u32);
	// addi r4,r1,468
	ctx.r4.s64 = ctx.r1.s64 + 468;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x829558a0
	ctx.lr = 0x829574F0;
	sub_829558A0(ctx, base);
	// addi r6,r1,256
	ctx.r6.s64 = ctx.r1.s64 + 256;
	// addi r5,r1,272
	ctx.r5.s64 = ctx.r1.s64 + 272;
	// addi r4,r1,276
	ctx.r4.s64 = ctx.r1.s64 + 276;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x829558a0
	ctx.lr = 0x82957504;
	sub_829558A0(ctx, base);
	// addi r6,r1,208
	ctx.r6.s64 = ctx.r1.s64 + 208;
	// addi r5,r1,224
	ctx.r5.s64 = ctx.r1.s64 + 224;
	// addi r4,r1,228
	ctx.r4.s64 = ctx.r1.s64 + 228;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x829558a0
	ctx.lr = 0x82957518;
	sub_829558A0(ctx, base);
	// addi r6,r1,304
	ctx.r6.s64 = ctx.r1.s64 + 304;
	// addi r5,r1,320
	ctx.r5.s64 = ctx.r1.s64 + 320;
	// addi r4,r1,324
	ctx.r4.s64 = ctx.r1.s64 + 324;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x829558a0
	ctx.lr = 0x8295752C;
	sub_829558A0(ctx, base);
	// mr r14,r18
	ctx.r14.u64 = ctx.r18.u64;
	// cmplwi cr6,r25,0
	ctx.cr6.compare<uint32_t>(ctx.r25.u32, 0, ctx.xer);
	// stw r14,104(r1)
	REX_STORE_U32(ctx.r1.u32 + 104, ctx.r14.u32);
	// beq cr6,0x82959130
	if (ctx.cr6.eq) goto loc_82959130;
	// lis r11,-32228
	ctx.r11.s64 = -2112094208;
	// lwz r15,1404(r1)
	ctx.r15.u64 = REX_LOAD_U32(ctx.r1.u32 + 1404);
	// lwz r29,1396(r1)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r1.u32 + 1396);
	// li r28,1
	ctx.r28.s64 = 1;
	// addi r31,r11,25592
	ctx.r31.s64 = ctx.r11.s64 + 25592;
	// lis r11,-32227
	ctx.r11.s64 = -2112028672;
	// addi r11,r11,-31496
	ctx.r11.s64 = ctx.r11.s64 + -31496;
	// stw r11,124(r1)
	REX_STORE_U32(ctx.r1.u32 + 124, ctx.r11.u32);
loc_8295755C:
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,480
	ctx.r3.s64 = ctx.r1.s64 + 480;
	// bl 0x829560a0
	ctx.lr = 0x82957568;
	sub_829560A0(ctx, base);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,336
	ctx.r3.s64 = ctx.r1.s64 + 336;
	// bl 0x829560a0
	ctx.lr = 0x82957574;
	sub_829560A0(ctx, base);
	// lwz r11,120(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 120);
	// addi r5,r1,768
	ctx.r5.s64 = ctx.r1.s64 + 768;
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,128
	ctx.r3.s64 = ctx.r1.s64 + 128;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x8295758C;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,432
	ctx.r3.s64 = ctx.r1.s64 + 432;
	// bl 0x82956a88
	ctx.lr = 0x82957598;
	sub_82956A88(ctx, base);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,240
	ctx.r3.s64 = ctx.r1.s64 + 240;
	// bl 0x82956c60
	ctx.lr = 0x829575A4;
	sub_82956C60(ctx, base);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,192
	ctx.r3.s64 = ctx.r1.s64 + 192;
	// bl 0x82956c60
	ctx.lr = 0x829575B0;
	sub_82956C60(ctx, base);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,384
	ctx.r3.s64 = ctx.r1.s64 + 384;
	// bl 0x82956ed0
	ctx.lr = 0x829575BC;
	sub_82956ED0(ctx, base);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,592
	ctx.r3.s64 = ctx.r1.s64 + 592;
	// bl 0x82956ed0
	ctx.lr = 0x829575C8;
	sub_82956ED0(ctx, base);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,288
	ctx.r3.s64 = ctx.r1.s64 + 288;
	// bl 0x82956878
	ctx.lr = 0x829575D4;
	sub_82956878(ctx, base);
	// lwz r3,84(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 84);
	// stw r17,116(r1)
	REX_STORE_U32(ctx.r1.u32 + 116, ctx.r17.u32);
	// cmplwi cr6,r17,0
	ctx.cr6.compare<uint32_t>(ctx.r17.u32, 0, ctx.xer);
	// beq cr6,0x829590dc
	if (ctx.cr6.eq) goto loc_829590DC;
	// lwz r25,88(r1)
	ctx.r25.u64 = REX_LOAD_U32(ctx.r1.u32 + 88);
	// lwz r29,80(r1)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// lwz r11,128(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 128);
	// lwz r27,432(r1)
	ctx.r27.u64 = REX_LOAD_U32(ctx.r1.u32 + 432);
	// lwz r21,240(r1)
	ctx.r21.u64 = REX_LOAD_U32(ctx.r1.u32 + 240);
	// lwz r22,192(r1)
	ctx.r22.u64 = REX_LOAD_U32(ctx.r1.u32 + 192);
	// lwz r16,384(r1)
	ctx.r16.u64 = REX_LOAD_U32(ctx.r1.u32 + 384);
	// lwz r19,288(r1)
	ctx.r19.u64 = REX_LOAD_U32(ctx.r1.u32 + 288);
loc_82957604:
	// lwz r9,480(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 480);
	// lbz r10,0(r9)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r9.u32 + 0);
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// cmplwi cr6,r10,9
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 9, ctx.xer);
	// stw r9,480(r1)
	REX_STORE_U32(ctx.r1.u32 + 480, ctx.r9.u32);
	// bgt cr6,0x829590a0
	if (ctx.cr6.gt) goto loc_829590A0;
	// lis r12,-32107
	ctx.r12.s64 = -2104164352;
	// addi r12,r12,30260
	ctx.r12.s64 = ctx.r12.s64 + 30260;
	// rlwinm r0,r10,2,0,29
	ctx.r0.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// lwzx r0,r12,r0
	ctx.r0.u64 = REX_LOAD_U32(ctx.r12.u32 + ctx.r0.u32);
	// mtctr r0
	ctx.ctr.u64 = ctx.r0.u64;
	// bctr 
	switch (ctx.r10.u32) {
	case 0:
		goto loc_8295765C;
	case 1:
		goto loc_829576EC;
	case 2:
		goto loc_829582C4;
	case 3:
		goto loc_8295866C;
	case 4:
		goto loc_8295880C;
	case 5:
		goto loc_829588B4;
	case 6:
		goto loc_82958958;
	case 7:
		goto loc_829589D8;
	case 8:
		goto loc_82958AB4;
	case 9:
		goto loc_82958D68;
	default:
		__builtin_trap(); // Switch case out of range
	}
loc_8295765C:
	// add r10,r20,r30
	ctx.r10.u64 = ctx.r20.u64 + ctx.r30.u64;
	// lfd f0,0(r20)
	ctx.fpscr.disableFlushMode();
	ctx.f0.u64 = REX_LOAD_U64(ctx.r20.u32 + 0);
	// add r9,r26,r30
	ctx.r9.u64 = ctx.r26.u64 + ctx.r30.u64;
	// stfd f0,0(r26)
	REX_STORE_U64(ctx.r26.u32 + 0, ctx.f0.u64);
	// add r8,r10,r30
	ctx.r8.u64 = ctx.r10.u64 + ctx.r30.u64;
	// rlwinm r7,r24,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 31) & 0x7FFFFFFF;
	// lfd f0,0(r10)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r10.u32 + 0);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stfd f0,0(r9)
	REX_STORE_U64(ctx.r9.u32 + 0, ctx.f0.u64);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// lfd f0,0(r8)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r8.u32 + 0);
	// add r8,r10,r30
	ctx.r8.u64 = ctx.r10.u64 + ctx.r30.u64;
	// stfd f0,0(r10)
	REX_STORE_U64(ctx.r10.u32 + 0, ctx.f0.u64);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lfd f0,0(r9)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r9.u32 + 0);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stfd f0,0(r8)
	REX_STORE_U64(ctx.r8.u32 + 0, ctx.f0.u64);
	// add r8,r10,r30
	ctx.r8.u64 = ctx.r10.u64 + ctx.r30.u64;
	// lfd f0,0(r10)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r10.u32 + 0);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stfd f0,0(r9)
	REX_STORE_U64(ctx.r9.u32 + 0, ctx.f0.u64);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// lfd f0,0(r8)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r8.u32 + 0);
	// add r8,r10,r30
	ctx.r8.u64 = ctx.r10.u64 + ctx.r30.u64;
	// stfd f0,0(r10)
	REX_STORE_U64(ctx.r10.u32 + 0, ctx.f0.u64);
	// lfd f0,0(r9)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r9.u32 + 0);
	// stfd f0,0(r8)
	REX_STORE_U64(ctx.r8.u32 + 0, ctx.f0.u64);
	// lfdx f0,r9,r30
	ctx.f0.u64 = REX_LOAD_U64(ctx.r9.u32 + ctx.r30.u32);
	// stfdx f0,r8,r30
	REX_STORE_U64(ctx.r8.u32 + ctx.r30.u32, ctx.f0.u64);
	// lbzx r10,r7,r23
	ctx.r10.u64 = REX_LOAD_U8(ctx.r7.u32 + ctx.r23.u32);
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// bne cr6,0x829590a0
	if (!ctx.cr6.eq) goto loc_829590A0;
	// lwz r10,100(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 100);
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
	// stw r10,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r10.u32);
	// b 0x829590a0
	goto loc_829590A0;
loc_829576EC:
	// rlwinm r10,r14,0,28,28
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r14.u32 | (ctx.r14.u64 << 32), 0) & 0x8;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// bne cr6,0x829582a0
	if (!ctx.cr6.eq) goto loc_829582A0;
	// lwz r9,336(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 336);
	// lbz r10,0(r9)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r9.u32 + 0);
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// addi r10,r10,-3
	ctx.r10.s64 = ctx.r10.s64 + -3;
	// cmplwi cr6,r10,6
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 6, ctx.xer);
	// stw r9,336(r1)
	REX_STORE_U32(ctx.r1.u32 + 336, ctx.r9.u32);
	// bgt cr6,0x829582a0
	if (ctx.cr6.gt) goto loc_829582A0;
	// lis r12,-32107
	ctx.r12.s64 = -2104164352;
	// addi r12,r12,30508
	ctx.r12.s64 = ctx.r12.s64 + 30508;
	// rlwinm r0,r10,2,0,29
	ctx.r0.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// lwzx r0,r12,r0
	ctx.r0.u64 = REX_LOAD_U32(ctx.r12.u32 + ctx.r0.u32);
	// mtctr r0
	ctx.ctr.u64 = ctx.r0.u64;
	// bctr 
	switch (ctx.r10.u32) {
	case 0:
		goto loc_82957748;
	case 1:
		goto loc_829582A0;
	case 2:
		goto loc_82957AB4;
	case 3:
		goto loc_82957B50;
	case 4:
		goto loc_829582A0;
	case 5:
		goto loc_82957CAC;
	case 6:
		goto loc_82957FF0;
	default:
		__builtin_trap(); // Switch case out of range
	}
loc_82957748:
	// cmplwi cr6,r25,4
	ctx.cr6.compare<uint32_t>(ctx.r25.u32, 4, ctx.xer);
	// bge cr6,0x82957774
	if (!ctx.cr6.lt) goto loc_82957774;
	// lwz r10,0(r3)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);
	// subfic r8,r25,4
	ctx.xer.ca = ctx.r25.u32 <= 4;
	ctx.r8.u64 = static_cast<uint64_t>(4) - ctx.r25.u64;
	// addi r4,r25,28
	ctx.r4.s64 = ctx.r25.s64 + 28;
	// addi r3,r3,4
	ctx.r3.s64 = ctx.r3.s64 + 4;
	// slw r9,r10,r25
	ctx.r9.u64 = ctx.r25.u8 & 0x20 ? 0 : (ctx.r10.u32 << (ctx.r25.u8 & 0x3F));
	// srw r5,r10,r8
	ctx.r5.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r8.u8 & 0x3F));
	// or r9,r9,r29
	ctx.r9.u64 = ctx.r9.u64 | ctx.r29.u64;
	// clrlwi r9,r9,28
	ctx.r9.u64 = ctx.r9.u32 & 0xF;
	// b 0x82957780
	goto loc_82957780;
loc_82957774:
	// clrlwi r9,r29,28
	ctx.r9.u64 = ctx.r29.u32 & 0xF;
	// rlwinm r5,r29,28,4,31
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 28) & 0xFFFFFFF;
	// addi r4,r25,-4
	ctx.r4.s64 = ctx.r25.s64 + -4;
loc_82957780:
	// lwz r8,124(r1)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r1.u32 + 124);
	// rlwinm r9,r9,6,0,25
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 6) & 0xFFFFFFC0;
	// mr r10,r18
	ctx.r10.u64 = ctx.r18.u64;
	// add r7,r9,r8
	ctx.r7.u64 = ctx.r9.u64 + ctx.r8.u64;
loc_82957790:
	// cmplwi cr6,r4,0
	ctx.cr6.compare<uint32_t>(ctx.r4.u32, 0, ctx.xer);
	// bne cr6,0x829577ac
	if (!ctx.cr6.eq) goto loc_829577AC;
	// lwz r9,0(r3)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);
	// li r4,31
	ctx.r4.s64 = 31;
	// addi r3,r3,4
	ctx.r3.s64 = ctx.r3.s64 + 4;
	// rlwinm r5,r9,31,1,31
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x829577b8
	goto loc_829577B8;
loc_829577AC:
	// mr r9,r5
	ctx.r9.u64 = ctx.r5.u64;
	// rlwinm r5,r5,31,1,31
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r4,r4,-1
	ctx.r4.s64 = ctx.r4.s64 + -1;
loc_829577B8:
	// clrlwi r9,r9,31
	ctx.r9.u64 = ctx.r9.u32 & 0x1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// beq cr6,0x82957804
	if (ctx.cr6.eq) goto loc_82957804;
	// lbz r8,0(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// lbz r9,0(r19)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r19.u32 + 0);
	// addi r19,r19,1
	ctx.r19.s64 = ctx.r19.s64 + 1;
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// cmpwi cr6,r9,0
	ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
	// beq cr6,0x82957840
	if (ctx.cr6.eq) goto loc_82957840;
loc_829577E4:
	// lbzx r6,r10,r7
	ctx.r6.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r7.u32);
	// addi r29,r1,528
	ctx.r29.s64 = ctx.r1.s64 + 528;
	// addi r9,r9,-1
	ctx.r9.s64 = ctx.r9.s64 + -1;
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
	// cmpwi cr6,r9,0
	ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
	// stbx r8,r6,r29
	REX_STORE_U8(ctx.r6.u32 + ctx.r29.u32, ctx.r8.u8);
	// bne cr6,0x829577e4
	if (!ctx.cr6.eq) goto loc_829577E4;
	// b 0x82957840
	goto loc_82957840;
loc_82957804:
	// lbz r9,0(r19)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r19.u32 + 0);
	// addi r19,r19,1
	ctx.r19.s64 = ctx.r19.s64 + 1;
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// cmpwi cr6,r9,0
	ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
	// beq cr6,0x82957840
	if (ctx.cr6.eq) goto loc_82957840;
loc_82957818:
	// lbz r6,0(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// addi r29,r1,528
	ctx.r29.s64 = ctx.r1.s64 + 528;
	// lbzx r8,r10,r7
	ctx.r8.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r7.u32);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// addi r9,r9,-1
	ctx.r9.s64 = ctx.r9.s64 + -1;
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
	// cmpwi cr6,r9,0
	ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// stbx r6,r8,r29
	REX_STORE_U8(ctx.r8.u32 + ctx.r29.u32, ctx.r6.u8);
	// bne cr6,0x82957818
	if (!ctx.cr6.eq) goto loc_82957818;
loc_82957840:
	// cmpwi cr6,r10,63
	ctx.cr6.compare<int32_t>(ctx.r10.s32, 63, ctx.xer);
	// blt cr6,0x82957790
	if (ctx.cr6.lt) goto loc_82957790;
	// stw r19,288(r1)
	REX_STORE_U32(ctx.r1.u32 + 288, ctx.r19.u32);
	// bne cr6,0x82957868
	if (!ctx.cr6.eq) goto loc_82957868;
	// lbz r10,0(r11)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// addi r8,r1,528
	ctx.r8.s64 = ctx.r1.s64 + 528;
	// lbz r9,63(r7)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r7.u32 + 63);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// stbx r10,r9,r8
	REX_STORE_U8(ctx.r9.u32 + ctx.r8.u32, ctx.r10.u8);
loc_82957868:
	// mr r9,r26
	ctx.r9.u64 = ctx.r26.u64;
	// add r8,r26,r30
	ctx.r8.u64 = ctx.r26.u64 + ctx.r30.u64;
	// addi r10,r1,532
	ctx.r10.s64 = ctx.r1.s64 + 532;
	// li r6,2
	ctx.r6.s64 = 2;
loc_82957878:
	// lhz r7,-4(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + -4);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,-2(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + -2);
	// stw r29,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r25.u32);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,0(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 0);
	// stw r29,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r25.u32);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,2(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 2);
	// stw r29,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r25.u32);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,4(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 4);
	// stw r29,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r29.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r25,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r25.u32);
	// rlwinm r8,r7,8,0,15
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r8,r8,r7
	ctx.r8.u64 = ctx.r8.u64 | ctx.r7.u64;
	// rlwimi r7,r8,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,6(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 6);
	// stw r29,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r25.u32);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,8(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 8);
	// stw r29,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r25.u32);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,10(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 10);
	// stw r29,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r25.u32);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// stw r7,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r7.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// lhz r7,12(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 12);
	// stw r29,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r29.u32);
	// rlwinm r8,r7,8,0,15
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r29,r8,r7
	ctx.r29.u64 = ctx.r8.u64 | ctx.r7.u64;
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,14(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 14);
	// stw r29,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r25.u32);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,16(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 16);
	// addi r6,r6,-1
	ctx.r6.s64 = ctx.r6.s64 + -1;
	// stw r29,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r25.u32);
	// cmplwi cr6,r6,0
	ctx.cr6.compare<uint32_t>(ctx.r6.u32, 0, ctx.xer);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,18(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 18);
	// stw r29,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r25.u32);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,20(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 20);
	// stw r29,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r29.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r25,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r25.u32);
	// rlwinm r8,r7,8,0,15
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r8,r8,r7
	ctx.r8.u64 = ctx.r8.u64 | ctx.r7.u64;
	// rlwimi r7,r8,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,22(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 22);
	// stw r29,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r25.u32);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,24(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 24);
	// stw r29,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r25.u32);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// mr r25,r7
	ctx.r25.u64 = ctx.r7.u64;
	// lhz r7,26(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 26);
	// addi r10,r10,32
	ctx.r10.s64 = ctx.r10.s64 + 32;
	// stw r29,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r29.u32);
	// rlwinm r29,r7,8,0,15
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// stw r25,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r25.u32);
	// or r29,r29,r7
	ctx.r29.u64 = ctx.r29.u64 | ctx.r7.u64;
	// rlwimi r7,r29,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r7.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r7,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r7.u32);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// bne cr6,0x82957878
	if (!ctx.cr6.eq) goto loc_82957878;
	// mr r29,r5
	ctx.r29.u64 = ctx.r5.u64;
	// stw r3,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r3.u32);
	// mr r25,r4
	ctx.r25.u64 = ctx.r4.u64;
	// stw r29,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r29.u32);
	// stw r25,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r25.u32);
	// b 0x829582a0
	goto loc_829582A0;
loc_82957AB4:
	// lhz r11,0(r16)
	ctx.r11.u64 = REX_LOAD_U16(ctx.r16.u32 + 0);
	// addi r16,r16,2
	ctx.r16.s64 = ctx.r16.s64 + 2;
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,640
	ctx.r3.s64 = ctx.r1.s64 + 640;
	// sth r11,640(r1)
	REX_STORE_U16(ctx.r1.u32 + 640, ctx.r11.u16);
	// stw r16,384(r1)
	REX_STORE_U32(ctx.r1.u32 + 384, ctx.r16.u32);
	// bl 0x8295f208
	ctx.lr = 0x82957AD0;
	sub_8295F208(ctx, base);
	// lwz r11,88(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 88);
	// cmplwi cr6,r11,4
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 4, ctx.xer);
	// bge cr6,0x82957b14
	if (!ctx.cr6.lt) goto loc_82957B14;
	// lwz r9,84(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 84);
	// subfic r8,r11,4
	ctx.xer.ca = ctx.r11.u32 <= 4;
	ctx.r8.u64 = static_cast<uint64_t>(4) - ctx.r11.u64;
	// lwz r10,0(r9)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r9.u32 + 0);
	// addi r9,r9,4
	ctx.r9.s64 = ctx.r9.s64 + 4;
	// stw r9,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r9.u32);
	// slw r9,r10,r11
	ctx.r9.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r10.u32 << (ctx.r11.u8 & 0x3F));
	// addi r11,r11,28
	ctx.r11.s64 = ctx.r11.s64 + 28;
	// stw r11,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r11.u32);
	// lwz r11,80(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// or r11,r9,r11
	ctx.r11.u64 = ctx.r9.u64 | ctx.r11.u64;
	// clrlwi r6,r11,28
	ctx.r6.u64 = ctx.r11.u32 & 0xF;
	// srw r11,r10,r8
	ctx.r11.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r8.u8 & 0x3F));
	// stw r11,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r11.u32);
	// b 0x82957b2c
	goto loc_82957B2C;
loc_82957B14:
	// lwz r10,80(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// addi r11,r11,-4
	ctx.r11.s64 = ctx.r11.s64 + -4;
	// clrlwi r6,r10,28
	ctx.r6.u64 = ctx.r10.u32 & 0xF;
	// rlwinm r10,r10,28,4,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 28) & 0xFFFFFFF;
	// stw r11,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r11.u32);
	// stw r10,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r10.u32);
loc_82957B2C:
	// addi r5,r1,640
	ctx.r5.s64 = ctx.r1.s64 + 640;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r26
	ctx.r3.u64 = ctx.r26.u64;
	// bl 0x8295ee68
	ctx.lr = 0x82957B3C;
	sub_8295EE68(ctx, base);
	// lwz r25,88(r1)
	ctx.r25.u64 = REX_LOAD_U32(ctx.r1.u32 + 88);
	// lwz r3,84(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 84);
	// lwz r29,80(r1)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// lwz r11,128(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 128);
	// b 0x829582a0
	goto loc_829582A0;
loc_82957B50:
	// lbz r10,0(r11)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// add r9,r26,r30
	ctx.r9.u64 = ctx.r26.u64 + ctx.r30.u64;
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// rotlwi r7,r10,8
	ctx.r7.u64 = __builtin_rotateleft32(ctx.r10.u32, 8);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// or r10,r7,r10
	ctx.r10.u64 = ctx.r7.u64 | ctx.r10.u64;
	// rlwinm r7,r10,16,0,15
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 16) & 0xFFFF0000;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// or r10,r7,r10
	ctx.r10.u64 = ctx.r7.u64 | ctx.r10.u64;
	// stw r10,0(r26)
	REX_STORE_U32(ctx.r26.u32 + 0, ctx.r10.u32);
	// stw r10,4(r26)
	REX_STORE_U32(ctx.r26.u32 + 4, ctx.r10.u32);
	// stw r10,8(r26)
	REX_STORE_U32(ctx.r26.u32 + 8, ctx.r10.u32);
	// stw r10,12(r26)
	REX_STORE_U32(ctx.r26.u32 + 12, ctx.r10.u32);
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// stw r10,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r10.u32);
	// stw r10,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r10.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r10,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r10.u32);
	// stw r10,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r10.u32);
	// stw r10,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r10.u32);
	// stw r10,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r10.u32);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// stw r10,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r10.u32);
	// stw r10,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r10.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r10,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r10.u32);
	// stw r10,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r10.u32);
	// stw r10,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r10.u32);
	// stw r10,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r10.u32);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// stw r10,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r10.u32);
	// stw r10,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r10.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r10,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r10.u32);
	// stw r10,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r10.u32);
	// stw r10,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r10.u32);
	// stw r10,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r10.u32);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// stw r10,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r10.u32);
	// stw r10,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r10.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r10,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r10.u32);
	// stw r10,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r10.u32);
	// stw r10,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r10.u32);
	// stw r10,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r10.u32);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// stw r10,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r10.u32);
	// stw r10,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r10.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r10,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r10.u32);
	// stw r10,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r10.u32);
	// stw r10,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r10.u32);
	// stw r10,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r10.u32);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// stw r10,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r10.u32);
	// stw r10,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r10.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r10,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r10.u32);
	// stw r10,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r10.u32);
	// stw r10,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r10.u32);
	// stw r10,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r10.u32);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// stw r10,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r10.u32);
	// stw r10,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r10.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r10,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r10.u32);
	// stw r10,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r10.u32);
	// stw r10,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r10.u32);
	// stw r10,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r10.u32);
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// stw r10,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r10.u32);
	// stw r10,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r10.u32);
	// b 0x829582a0
	goto loc_829582A0;
loc_82957CAC:
	// lbz r10,0(r11)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// mr r8,r26
	ctx.r8.u64 = ctx.r26.u64;
	// rotlwi r6,r10,8
	ctx.r6.u64 = __builtin_rotateleft32(ctx.r10.u32, 8);
	// add r7,r26,r30
	ctx.r7.u64 = ctx.r26.u64 + ctx.r30.u64;
	// or r10,r6,r10
	ctx.r10.u64 = ctx.r6.u64 | ctx.r10.u64;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// li r5,2
	ctx.r5.s64 = 2;
	// lbz r9,0(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// rlwinm r6,r10,16,0,15
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 16) & 0xFFFF0000;
	// rotlwi r4,r9,8
	ctx.r4.u64 = __builtin_rotateleft32(ctx.r9.u32, 8);
	// or r10,r6,r10
	ctx.r10.u64 = ctx.r6.u64 | ctx.r10.u64;
	// or r9,r4,r9
	ctx.r9.u64 = ctx.r4.u64 | ctx.r9.u64;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// rlwinm r4,r9,16,0,15
	ctx.r4.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 16) & 0xFFFF0000;
	// or r9,r4,r9
	ctx.r9.u64 = ctx.r4.u64 | ctx.r9.u64;
loc_82957CF0:
	// lbz r6,0(r27)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r27.u32 + 0);
	// addi r24,r31,128
	ctx.r24.s64 = ctx.r31.s64 + 128;
	// addi r23,r31,144
	ctx.r23.s64 = ctx.r31.s64 + 144;
	// rlwinm r29,r6,2,28,29
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xC;
	// rlwinm r28,r6,0,28,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 0) & 0xC;
	// addi r22,r31,128
	ctx.r22.s64 = ctx.r31.s64 + 128;
	// addi r21,r31,144
	ctx.r21.s64 = ctx.r31.s64 + 144;
	// addi r4,r27,1
	ctx.r4.s64 = ctx.r27.s64 + 1;
	// lwzx r24,r29,r24
	ctx.r24.u64 = REX_LOAD_U32(ctx.r29.u32 + ctx.r24.u32);
	// rlwinm r27,r6,30,28,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 30) & 0xC;
	// lwzx r29,r29,r23
	ctx.r29.u64 = REX_LOAD_U32(ctx.r29.u32 + ctx.r23.u32);
	// addi r20,r31,128
	ctx.r20.s64 = ctx.r31.s64 + 128;
	// and r24,r24,r9
	ctx.r24.u64 = ctx.r24.u64 & ctx.r9.u64;
	// and r29,r29,r10
	ctx.r29.u64 = ctx.r29.u64 & ctx.r10.u64;
	// addi r19,r31,144
	ctx.r19.s64 = ctx.r31.s64 + 144;
	// or r29,r24,r29
	ctx.r29.u64 = ctx.r24.u64 | ctx.r29.u64;
	// rlwinm r6,r6,28,4,29
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 28) & 0xFFFFFFC;
	// addi r18,r31,128
	ctx.r18.s64 = ctx.r31.s64 + 128;
	// addi r17,r31,144
	ctx.r17.s64 = ctx.r31.s64 + 144;
	// addi r16,r31,128
	ctx.r16.s64 = ctx.r31.s64 + 128;
	// stw r29,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r29.u32);
	// addi r15,r31,144
	ctx.r15.s64 = ctx.r31.s64 + 144;
	// stw r29,0(r7)
	REX_STORE_U32(ctx.r7.u32 + 0, ctx.r29.u32);
	// addi r14,r31,128
	ctx.r14.s64 = ctx.r31.s64 + 128;
	// lwzx r29,r28,r22
	ctx.r29.u64 = REX_LOAD_U32(ctx.r28.u32 + ctx.r22.u32);
	// addi r24,r31,144
	ctx.r24.s64 = ctx.r31.s64 + 144;
	// lwzx r28,r28,r21
	ctx.r28.u64 = REX_LOAD_U32(ctx.r28.u32 + ctx.r21.u32);
	// addi r23,r31,128
	ctx.r23.s64 = ctx.r31.s64 + 128;
	// and r29,r29,r9
	ctx.r29.u64 = ctx.r29.u64 & ctx.r9.u64;
	// and r28,r28,r10
	ctx.r28.u64 = ctx.r28.u64 & ctx.r10.u64;
	// addi r22,r31,144
	ctx.r22.s64 = ctx.r31.s64 + 144;
	// or r29,r29,r28
	ctx.r29.u64 = ctx.r29.u64 | ctx.r28.u64;
	// addi r21,r31,144
	ctx.r21.s64 = ctx.r31.s64 + 144;
	// stw r29,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r29.u32);
	// stw r29,4(r7)
	REX_STORE_U32(ctx.r7.u32 + 4, ctx.r29.u32);
	// lwzx r29,r27,r20
	ctx.r29.u64 = REX_LOAD_U32(ctx.r27.u32 + ctx.r20.u32);
	// addi r20,r31,128
	ctx.r20.s64 = ctx.r31.s64 + 128;
	// lwzx r28,r27,r19
	ctx.r28.u64 = REX_LOAD_U32(ctx.r27.u32 + ctx.r19.u32);
	// addi r19,r31,144
	ctx.r19.s64 = ctx.r31.s64 + 144;
	// and r29,r29,r9
	ctx.r29.u64 = ctx.r29.u64 & ctx.r9.u64;
	// and r28,r28,r10
	ctx.r28.u64 = ctx.r28.u64 & ctx.r10.u64;
	// or r29,r29,r28
	ctx.r29.u64 = ctx.r29.u64 | ctx.r28.u64;
	// stw r29,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r29.u32);
	// stw r29,8(r7)
	REX_STORE_U32(ctx.r7.u32 + 8, ctx.r29.u32);
	// lwzx r29,r6,r18
	ctx.r29.u64 = REX_LOAD_U32(ctx.r6.u32 + ctx.r18.u32);
	// addi r18,r31,128
	ctx.r18.s64 = ctx.r31.s64 + 128;
	// lwzx r6,r6,r17
	ctx.r6.u64 = REX_LOAD_U32(ctx.r6.u32 + ctx.r17.u32);
	// addi r17,r31,144
	ctx.r17.s64 = ctx.r31.s64 + 144;
	// and r29,r29,r9
	ctx.r29.u64 = ctx.r29.u64 & ctx.r9.u64;
	// and r6,r6,r10
	ctx.r6.u64 = ctx.r6.u64 & ctx.r10.u64;
	// or r6,r29,r6
	ctx.r6.u64 = ctx.r29.u64 | ctx.r6.u64;
	// stw r6,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r6.u32);
	// add r8,r7,r30
	ctx.r8.u64 = ctx.r7.u64 + ctx.r30.u64;
	// stw r6,12(r7)
	REX_STORE_U32(ctx.r7.u32 + 12, ctx.r6.u32);
	// lbz r6,0(r4)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r4.u32 + 0);
	// add r7,r8,r30
	ctx.r7.u64 = ctx.r8.u64 + ctx.r30.u64;
	// addi r4,r4,1
	ctx.r4.s64 = ctx.r4.s64 + 1;
	// rlwinm r29,r6,2,28,29
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xC;
	// rlwinm r28,r6,0,28,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 0) & 0xC;
	// lwzx r27,r29,r16
	ctx.r27.u64 = REX_LOAD_U32(ctx.r29.u32 + ctx.r16.u32);
	// addi r16,r31,128
	ctx.r16.s64 = ctx.r31.s64 + 128;
	// lwzx r29,r29,r15
	ctx.r29.u64 = REX_LOAD_U32(ctx.r29.u32 + ctx.r15.u32);
	// addi r15,r31,144
	ctx.r15.s64 = ctx.r31.s64 + 144;
	// and r27,r27,r9
	ctx.r27.u64 = ctx.r27.u64 & ctx.r9.u64;
	// and r29,r29,r10
	ctx.r29.u64 = ctx.r29.u64 & ctx.r10.u64;
	// or r29,r27,r29
	ctx.r29.u64 = ctx.r27.u64 | ctx.r29.u64;
	// stw r29,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r29.u32);
	// stw r29,0(r7)
	REX_STORE_U32(ctx.r7.u32 + 0, ctx.r29.u32);
	// lwzx r29,r28,r14
	ctx.r29.u64 = REX_LOAD_U32(ctx.r28.u32 + ctx.r14.u32);
	// lwzx r28,r28,r24
	ctx.r28.u64 = REX_LOAD_U32(ctx.r28.u32 + ctx.r24.u32);
	// addi r24,r31,128
	ctx.r24.s64 = ctx.r31.s64 + 128;
	// and r27,r29,r9
	ctx.r27.u64 = ctx.r29.u64 & ctx.r9.u64;
	// and r28,r28,r10
	ctx.r28.u64 = ctx.r28.u64 & ctx.r10.u64;
	// rlwinm r29,r6,30,28,29
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 30) & 0xC;
	// or r28,r27,r28
	ctx.r28.u64 = ctx.r27.u64 | ctx.r28.u64;
	// rlwinm r6,r6,28,4,29
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 28) & 0xFFFFFFC;
	// stw r28,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r28.u32);
	// stw r28,4(r7)
	REX_STORE_U32(ctx.r7.u32 + 4, ctx.r28.u32);
	// lwzx r28,r29,r23
	ctx.r28.u64 = REX_LOAD_U32(ctx.r29.u32 + ctx.r23.u32);
	// lwzx r29,r29,r22
	ctx.r29.u64 = REX_LOAD_U32(ctx.r29.u32 + ctx.r22.u32);
	// and r28,r28,r9
	ctx.r28.u64 = ctx.r28.u64 & ctx.r9.u64;
	// and r29,r29,r10
	ctx.r29.u64 = ctx.r29.u64 & ctx.r10.u64;
	// addi r23,r31,144
	ctx.r23.s64 = ctx.r31.s64 + 144;
	// or r29,r28,r29
	ctx.r29.u64 = ctx.r28.u64 | ctx.r29.u64;
	// addi r22,r31,128
	ctx.r22.s64 = ctx.r31.s64 + 128;
	// addi r5,r5,-1
	ctx.r5.s64 = ctx.r5.s64 + -1;
	// cmplwi cr6,r5,0
	ctx.cr6.compare<uint32_t>(ctx.r5.u32, 0, ctx.xer);
	// stw r29,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r29.u32);
	// stw r29,8(r7)
	REX_STORE_U32(ctx.r7.u32 + 8, ctx.r29.u32);
	// lwzx r29,r6,r24
	ctx.r29.u64 = REX_LOAD_U32(ctx.r6.u32 + ctx.r24.u32);
	// lwzx r6,r6,r21
	ctx.r6.u64 = REX_LOAD_U32(ctx.r6.u32 + ctx.r21.u32);
	// addi r21,r31,144
	ctx.r21.s64 = ctx.r31.s64 + 144;
	// and r29,r29,r9
	ctx.r29.u64 = ctx.r29.u64 & ctx.r9.u64;
	// and r6,r6,r10
	ctx.r6.u64 = ctx.r6.u64 & ctx.r10.u64;
	// or r6,r29,r6
	ctx.r6.u64 = ctx.r29.u64 | ctx.r6.u64;
	// stw r6,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r6.u32);
	// add r8,r7,r30
	ctx.r8.u64 = ctx.r7.u64 + ctx.r30.u64;
	// stw r6,12(r7)
	REX_STORE_U32(ctx.r7.u32 + 12, ctx.r6.u32);
	// lbz r6,0(r4)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r4.u32 + 0);
	// add r7,r8,r30
	ctx.r7.u64 = ctx.r8.u64 + ctx.r30.u64;
	// addi r4,r4,1
	ctx.r4.s64 = ctx.r4.s64 + 1;
	// rlwinm r29,r6,2,28,29
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xC;
	// rlwinm r28,r6,0,28,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 0) & 0xC;
	// rlwinm r27,r6,30,28,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 30) & 0xC;
	// rlwinm r6,r6,28,4,29
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 28) & 0xFFFFFFC;
	// lwzx r24,r29,r20
	ctx.r24.u64 = REX_LOAD_U32(ctx.r29.u32 + ctx.r20.u32);
	// addi r20,r31,128
	ctx.r20.s64 = ctx.r31.s64 + 128;
	// lwzx r29,r29,r19
	ctx.r29.u64 = REX_LOAD_U32(ctx.r29.u32 + ctx.r19.u32);
	// addi r19,r31,144
	ctx.r19.s64 = ctx.r31.s64 + 144;
	// and r24,r24,r9
	ctx.r24.u64 = ctx.r24.u64 & ctx.r9.u64;
	// and r29,r29,r10
	ctx.r29.u64 = ctx.r29.u64 & ctx.r10.u64;
	// or r29,r24,r29
	ctx.r29.u64 = ctx.r24.u64 | ctx.r29.u64;
	// addi r24,r31,128
	ctx.r24.s64 = ctx.r31.s64 + 128;
	// stw r29,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r29.u32);
	// stw r29,0(r7)
	REX_STORE_U32(ctx.r7.u32 + 0, ctx.r29.u32);
	// lwzx r29,r28,r18
	ctx.r29.u64 = REX_LOAD_U32(ctx.r28.u32 + ctx.r18.u32);
	// addi r18,r31,128
	ctx.r18.s64 = ctx.r31.s64 + 128;
	// lwzx r28,r28,r17
	ctx.r28.u64 = REX_LOAD_U32(ctx.r28.u32 + ctx.r17.u32);
	// addi r17,r31,144
	ctx.r17.s64 = ctx.r31.s64 + 144;
	// and r29,r29,r9
	ctx.r29.u64 = ctx.r29.u64 & ctx.r9.u64;
	// and r28,r28,r10
	ctx.r28.u64 = ctx.r28.u64 & ctx.r10.u64;
	// or r29,r29,r28
	ctx.r29.u64 = ctx.r29.u64 | ctx.r28.u64;
	// stw r29,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r29.u32);
	// stw r29,4(r7)
	REX_STORE_U32(ctx.r7.u32 + 4, ctx.r29.u32);
	// lwzx r29,r27,r16
	ctx.r29.u64 = REX_LOAD_U32(ctx.r27.u32 + ctx.r16.u32);
	// lwzx r28,r27,r15
	ctx.r28.u64 = REX_LOAD_U32(ctx.r27.u32 + ctx.r15.u32);
	// addi r27,r31,144
	ctx.r27.s64 = ctx.r31.s64 + 144;
	// and r29,r29,r9
	ctx.r29.u64 = ctx.r29.u64 & ctx.r9.u64;
	// and r28,r28,r10
	ctx.r28.u64 = ctx.r28.u64 & ctx.r10.u64;
	// or r29,r29,r28
	ctx.r29.u64 = ctx.r29.u64 | ctx.r28.u64;
	// addi r28,r31,128
	ctx.r28.s64 = ctx.r31.s64 + 128;
	// stw r29,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r29.u32);
	// stw r29,8(r7)
	REX_STORE_U32(ctx.r7.u32 + 8, ctx.r29.u32);
	// lwzx r29,r6,r28
	ctx.r29.u64 = REX_LOAD_U32(ctx.r6.u32 + ctx.r28.u32);
	// lwzx r6,r6,r27
	ctx.r6.u64 = REX_LOAD_U32(ctx.r6.u32 + ctx.r27.u32);
	// addi r27,r4,1
	ctx.r27.s64 = ctx.r4.s64 + 1;
	// and r29,r29,r9
	ctx.r29.u64 = ctx.r29.u64 & ctx.r9.u64;
	// and r6,r6,r10
	ctx.r6.u64 = ctx.r6.u64 & ctx.r10.u64;
	// or r6,r29,r6
	ctx.r6.u64 = ctx.r29.u64 | ctx.r6.u64;
	// stw r6,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r6.u32);
	// add r8,r7,r30
	ctx.r8.u64 = ctx.r7.u64 + ctx.r30.u64;
	// stw r6,12(r7)
	REX_STORE_U32(ctx.r7.u32 + 12, ctx.r6.u32);
	// lbz r6,0(r4)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r4.u32 + 0);
	// add r7,r8,r30
	ctx.r7.u64 = ctx.r8.u64 + ctx.r30.u64;
	// rlwinm r4,r6,2,28,29
	ctx.r4.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xC;
	// rlwinm r29,r6,0,28,29
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 0) & 0xC;
	// rlwinm r28,r6,30,28,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 30) & 0xC;
	// rlwinm r6,r6,28,4,29
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 28) & 0xFFFFFFC;
	// lwzx r24,r4,r24
	ctx.r24.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r24.u32);
	// lwzx r4,r4,r23
	ctx.r4.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r23.u32);
	// and r24,r24,r9
	ctx.r24.u64 = ctx.r24.u64 & ctx.r9.u64;
	// and r4,r4,r10
	ctx.r4.u64 = ctx.r4.u64 & ctx.r10.u64;
	// or r4,r24,r4
	ctx.r4.u64 = ctx.r24.u64 | ctx.r4.u64;
	// stw r4,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r4.u32);
	// stw r4,0(r7)
	REX_STORE_U32(ctx.r7.u32 + 0, ctx.r4.u32);
	// lwzx r4,r29,r22
	ctx.r4.u64 = REX_LOAD_U32(ctx.r29.u32 + ctx.r22.u32);
	// lwzx r29,r29,r21
	ctx.r29.u64 = REX_LOAD_U32(ctx.r29.u32 + ctx.r21.u32);
	// and r4,r4,r9
	ctx.r4.u64 = ctx.r4.u64 & ctx.r9.u64;
	// and r29,r29,r10
	ctx.r29.u64 = ctx.r29.u64 & ctx.r10.u64;
	// or r4,r4,r29
	ctx.r4.u64 = ctx.r4.u64 | ctx.r29.u64;
	// stw r4,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r4.u32);
	// stw r4,4(r7)
	REX_STORE_U32(ctx.r7.u32 + 4, ctx.r4.u32);
	// lwzx r4,r28,r20
	ctx.r4.u64 = REX_LOAD_U32(ctx.r28.u32 + ctx.r20.u32);
	// lwzx r29,r28,r19
	ctx.r29.u64 = REX_LOAD_U32(ctx.r28.u32 + ctx.r19.u32);
	// and r4,r4,r9
	ctx.r4.u64 = ctx.r4.u64 & ctx.r9.u64;
	// and r29,r29,r10
	ctx.r29.u64 = ctx.r29.u64 & ctx.r10.u64;
	// or r4,r4,r29
	ctx.r4.u64 = ctx.r4.u64 | ctx.r29.u64;
	// stw r4,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r4.u32);
	// stw r4,8(r7)
	REX_STORE_U32(ctx.r7.u32 + 8, ctx.r4.u32);
	// lwzx r4,r6,r18
	ctx.r4.u64 = REX_LOAD_U32(ctx.r6.u32 + ctx.r18.u32);
	// lwzx r6,r6,r17
	ctx.r6.u64 = REX_LOAD_U32(ctx.r6.u32 + ctx.r17.u32);
	// and r4,r4,r9
	ctx.r4.u64 = ctx.r4.u64 & ctx.r9.u64;
	// and r6,r6,r10
	ctx.r6.u64 = ctx.r6.u64 & ctx.r10.u64;
	// or r6,r4,r6
	ctx.r6.u64 = ctx.r4.u64 | ctx.r6.u64;
	// stw r6,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r6.u32);
	// add r8,r7,r30
	ctx.r8.u64 = ctx.r7.u64 + ctx.r30.u64;
	// stw r6,12(r7)
	REX_STORE_U32(ctx.r7.u32 + 12, ctx.r6.u32);
	// add r7,r8,r30
	ctx.r7.u64 = ctx.r8.u64 + ctx.r30.u64;
	// bne cr6,0x82957cf0
	if (!ctx.cr6.eq) goto loc_82957CF0;
	// lwz r15,1404(r1)
	ctx.r15.u64 = REX_LOAD_U32(ctx.r1.u32 + 1404);
	// li r18,0
	ctx.r18.s64 = 0;
	// lwz r23,112(r1)
	ctx.r23.u64 = REX_LOAD_U32(ctx.r1.u32 + 112);
	// li r28,1
	ctx.r28.s64 = 1;
	// lwz r20,96(r1)
	ctx.r20.u64 = REX_LOAD_U32(ctx.r1.u32 + 96);
	// lwz r24,108(r1)
	ctx.r24.u64 = REX_LOAD_U32(ctx.r1.u32 + 108);
	// lwz r29,80(r1)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// lwz r17,116(r1)
	ctx.r17.u64 = REX_LOAD_U32(ctx.r1.u32 + 116);
	// lwz r19,288(r1)
	ctx.r19.u64 = REX_LOAD_U32(ctx.r1.u32 + 288);
	// lwz r16,384(r1)
	ctx.r16.u64 = REX_LOAD_U32(ctx.r1.u32 + 384);
	// lwz r22,192(r1)
	ctx.r22.u64 = REX_LOAD_U32(ctx.r1.u32 + 192);
	// lwz r21,240(r1)
	ctx.r21.u64 = REX_LOAD_U32(ctx.r1.u32 + 240);
	// b 0x829582a0
	goto loc_829582A0;
loc_82957FF0:
	// clrlwi r7,r11,30
	ctx.r7.u64 = ctx.r11.u32 & 0x3;
	// add r8,r26,r30
	ctx.r8.u64 = ctx.r26.u64 + ctx.r30.u64;
	// mr r10,r26
	ctx.r10.u64 = ctx.r26.u64;
	// rlwinm r9,r30,1,0,30
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 1) & 0xFFFFFFFE;
	// cmplwi cr6,r7,0
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 0, ctx.xer);
	// beq cr6,0x829580d0
	if (ctx.cr6.eq) goto loc_829580D0;
	// subf r4,r26,r8
	ctx.r4.u64 = ctx.r8.u64 - ctx.r26.u64;
	// li r7,8
	ctx.r7.s64 = 8;
loc_82958010:
	// lbz r6,0(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// add r8,r4,r10
	ctx.r8.u64 = ctx.r4.u64 + ctx.r10.u64;
	// lbz r5,1(r11)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r11.u32 + 1);
	// addi r7,r7,-1
	ctx.r7.s64 = ctx.r7.s64 + -1;
	// rotlwi r14,r6,8
	ctx.r14.u64 = __builtin_rotateleft32(ctx.r6.u32, 8);
	// cmplwi cr6,r7,0
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 0, ctx.xer);
	// or r6,r14,r6
	ctx.r6.u64 = ctx.r14.u64 | ctx.r6.u64;
	// rlwinm r6,r6,8,0,23
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// or r6,r6,r5
	ctx.r6.u64 = ctx.r6.u64 | ctx.r5.u64;
	// rlwinm r6,r6,8,0,23
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// or r6,r6,r5
	ctx.r6.u64 = ctx.r6.u64 | ctx.r5.u64;
	// stw r6,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r6.u32);
	// stw r6,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r6.u32);
	// lbz r6,2(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 2);
	// lbz r5,3(r11)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r11.u32 + 3);
	// rotlwi r14,r6,8
	ctx.r14.u64 = __builtin_rotateleft32(ctx.r6.u32, 8);
	// or r6,r14,r6
	ctx.r6.u64 = ctx.r14.u64 | ctx.r6.u64;
	// rlwinm r6,r6,8,0,23
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// or r6,r6,r5
	ctx.r6.u64 = ctx.r6.u64 | ctx.r5.u64;
	// rlwinm r6,r6,8,0,23
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// or r6,r6,r5
	ctx.r6.u64 = ctx.r6.u64 | ctx.r5.u64;
	// stw r6,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r6.u32);
	// stw r6,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r6.u32);
	// lbz r6,4(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 4);
	// lbz r5,5(r11)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r11.u32 + 5);
	// rotlwi r14,r6,8
	ctx.r14.u64 = __builtin_rotateleft32(ctx.r6.u32, 8);
	// or r6,r14,r6
	ctx.r6.u64 = ctx.r14.u64 | ctx.r6.u64;
	// rlwinm r6,r6,8,0,23
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// or r6,r6,r5
	ctx.r6.u64 = ctx.r6.u64 | ctx.r5.u64;
	// rlwinm r6,r6,8,0,23
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// or r6,r6,r5
	ctx.r6.u64 = ctx.r6.u64 | ctx.r5.u64;
	// stw r6,8(r10)
	REX_STORE_U32(ctx.r10.u32 + 8, ctx.r6.u32);
	// stw r6,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r6.u32);
	// lbz r6,6(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 6);
	// lbz r5,7(r11)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r11.u32 + 7);
	// addi r11,r11,8
	ctx.r11.s64 = ctx.r11.s64 + 8;
	// rotlwi r14,r6,8
	ctx.r14.u64 = __builtin_rotateleft32(ctx.r6.u32, 8);
	// or r6,r14,r6
	ctx.r6.u64 = ctx.r14.u64 | ctx.r6.u64;
	// rlwinm r6,r6,8,0,23
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// or r6,r6,r5
	ctx.r6.u64 = ctx.r6.u64 | ctx.r5.u64;
	// rlwinm r6,r6,8,0,23
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// or r6,r6,r5
	ctx.r6.u64 = ctx.r6.u64 | ctx.r5.u64;
	// stw r6,12(r10)
	REX_STORE_U32(ctx.r10.u32 + 12, ctx.r6.u32);
	// add r10,r9,r10
	ctx.r10.u64 = ctx.r9.u64 + ctx.r10.u64;
	// stw r6,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r6.u32);
	// bne cr6,0x82958010
	if (!ctx.cr6.eq) goto loc_82958010;
	// b 0x829582a0
	goto loc_829582A0;
loc_829580D0:
	// li r6,2
	ctx.r6.s64 = 2;
loc_829580D4:
	// lhz r7,0(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 0);
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r7.u32);
	// stw r7,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r7.u32);
	// lhz r7,2(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 2);
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r7.u32);
	// stw r7,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r7.u32);
	// lhz r7,4(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 4);
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,8(r10)
	REX_STORE_U32(ctx.r10.u32 + 8, ctx.r7.u32);
	// stw r7,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r7.u32);
	// lhz r7,6(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 6);
	// addi r11,r11,8
	ctx.r11.s64 = ctx.r11.s64 + 8;
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,12(r10)
	REX_STORE_U32(ctx.r10.u32 + 12, ctx.r7.u32);
	// add r10,r9,r10
	ctx.r10.u64 = ctx.r9.u64 + ctx.r10.u64;
	// stw r7,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r7.u32);
	// add r8,r9,r8
	ctx.r8.u64 = ctx.r9.u64 + ctx.r8.u64;
	// lhz r7,0(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 0);
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r7.u32);
	// stw r7,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r7.u32);
	// lhz r7,2(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 2);
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r7.u32);
	// stw r7,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r7.u32);
	// lhz r7,4(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 4);
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,8(r10)
	REX_STORE_U32(ctx.r10.u32 + 8, ctx.r7.u32);
	// stw r7,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r7.u32);
	// lhz r7,6(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 6);
	// addi r11,r11,8
	ctx.r11.s64 = ctx.r11.s64 + 8;
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,12(r10)
	REX_STORE_U32(ctx.r10.u32 + 12, ctx.r7.u32);
	// add r10,r9,r10
	ctx.r10.u64 = ctx.r9.u64 + ctx.r10.u64;
	// stw r7,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r7.u32);
	// add r8,r9,r8
	ctx.r8.u64 = ctx.r9.u64 + ctx.r8.u64;
	// lhz r7,0(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 0);
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r7.u32);
	// stw r7,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r7.u32);
	// lhz r7,2(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 2);
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r7.u32);
	// stw r7,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r7.u32);
	// lhz r7,4(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 4);
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,8(r10)
	REX_STORE_U32(ctx.r10.u32 + 8, ctx.r7.u32);
	// stw r7,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r7.u32);
	// lhz r7,6(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 6);
	// addi r11,r11,8
	ctx.r11.s64 = ctx.r11.s64 + 8;
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,12(r10)
	REX_STORE_U32(ctx.r10.u32 + 12, ctx.r7.u32);
	// stw r7,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r7.u32);
	// add r10,r9,r10
	ctx.r10.u64 = ctx.r9.u64 + ctx.r10.u64;
	// lhz r7,0(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 0);
	// add r8,r9,r8
	ctx.r8.u64 = ctx.r9.u64 + ctx.r8.u64;
	// addi r6,r6,-1
	ctx.r6.s64 = ctx.r6.s64 + -1;
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// cmplwi cr6,r6,0
	ctx.cr6.compare<uint32_t>(ctx.r6.u32, 0, ctx.xer);
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r7.u32);
	// stw r7,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r7.u32);
	// lhz r7,2(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 2);
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r7.u32);
	// stw r7,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r7.u32);
	// lhz r7,4(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 4);
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,8(r10)
	REX_STORE_U32(ctx.r10.u32 + 8, ctx.r7.u32);
	// stw r7,8(r8)
	REX_STORE_U32(ctx.r8.u32 + 8, ctx.r7.u32);
	// lhz r7,6(r11)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r11.u32 + 6);
	// addi r11,r11,8
	ctx.r11.s64 = ctx.r11.s64 + 8;
	// rlwinm r5,r7,8,0,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFF0000;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// rlwimi r7,r5,8,0,23
	ctx.r7.u64 = (__builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00) | (ctx.r7.u64 & 0xFFFFFFFF000000FF);
	// stw r7,12(r10)
	REX_STORE_U32(ctx.r10.u32 + 12, ctx.r7.u32);
	// add r10,r9,r10
	ctx.r10.u64 = ctx.r9.u64 + ctx.r10.u64;
	// stw r7,12(r8)
	REX_STORE_U32(ctx.r8.u32 + 12, ctx.r7.u32);
	// add r8,r9,r8
	ctx.r8.u64 = ctx.r9.u64 + ctx.r8.u64;
	// bne cr6,0x829580d4
	if (!ctx.cr6.eq) goto loc_829580D4;
loc_829582A0:
	// rlwinm r10,r24,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 31) & 0x7FFFFFFF;
	// add r24,r24,r15
	ctx.r24.u64 = ctx.r24.u64 + ctx.r15.u64;
	// addi r26,r26,8
	ctx.r26.s64 = ctx.r26.s64 + 8;
	// addi r20,r20,8
	ctx.r20.s64 = ctx.r20.s64 + 8;
	// addi r17,r17,-8
	ctx.r17.s64 = ctx.r17.s64 + -8;
	// stbx r28,r10,r23
	REX_STORE_U8(ctx.r10.u32 + ctx.r23.u32, ctx.r28.u8);
	// rlwinm r10,r24,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 31) & 0x7FFFFFFF;
	// stbx r28,r10,r23
	REX_STORE_U8(ctx.r10.u32 + ctx.r23.u32, ctx.r28.u8);
	// b 0x829590a0
	goto loc_829590A0;
loc_829582C4:
	// rlwinm r9,r24,31,1,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 31) & 0x7FFFFFFF;
	// add r10,r26,r30
	ctx.r10.u64 = ctx.r26.u64 + ctx.r30.u64;
	// stbx r28,r9,r23
	REX_STORE_U8(ctx.r9.u32 + ctx.r23.u32, ctx.r28.u8);
	// lbz r9,0(r22)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r22.u32 + 0);
	// addi r22,r22,1
	ctx.r22.s64 = ctx.r22.s64 + 1;
	// lbz r8,0(r21)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r21.u32 + 0);
	// addi r21,r21,1
	ctx.r21.s64 = ctx.r21.s64 + 1;
	// extsb r9,r9
	ctx.r9.s64 = ctx.r9.s8;
	// extsb r8,r8
	ctx.r8.s64 = ctx.r8.s8;
	// mullw r9,r9,r30
	ctx.r9.s64 = int64_t(ctx.r9.s32) * int64_t(ctx.r30.s32);
	// stw r22,192(r1)
	REX_STORE_U32(ctx.r1.u32 + 192, ctx.r22.u32);
	// stw r21,240(r1)
	REX_STORE_U32(ctx.r1.u32 + 240, ctx.r21.u32);
	// add r9,r9,r8
	ctx.r9.u64 = ctx.r9.u64 + ctx.r8.u64;
	// add r8,r9,r20
	ctx.r8.u64 = ctx.r9.u64 + ctx.r20.u64;
	// or r7,r8,r26
	ctx.r7.u64 = ctx.r8.u64 | ctx.r26.u64;
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// clrlwi r6,r7,29
	ctx.r6.u64 = ctx.r7.u32 & 0x7;
	// cmplwi cr6,r6,0
	ctx.cr6.compare<uint32_t>(ctx.r6.u32, 0, ctx.xer);
	// bne cr6,0x8295837c
	if (!ctx.cr6.eq) goto loc_8295837C;
	// lfd f0,0(r8)
	ctx.fpscr.disableFlushMode();
	ctx.f0.u64 = REX_LOAD_U64(ctx.r8.u32 + 0);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stfd f0,0(r26)
	REX_STORE_U64(ctx.r26.u32 + 0, ctx.f0.u64);
	// add r7,r10,r30
	ctx.r7.u64 = ctx.r10.u64 + ctx.r30.u64;
	// lfd f0,0(r9)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r9.u32 + 0);
	// stfd f0,0(r10)
	REX_STORE_U64(ctx.r10.u32 + 0, ctx.f0.u64);
	// add r10,r8,r30
	ctx.r10.u64 = ctx.r8.u64 + ctx.r30.u64;
	// add r9,r7,r30
	ctx.r9.u64 = ctx.r7.u64 + ctx.r30.u64;
	// lfd f0,0(r8)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r8.u32 + 0);
	// add r8,r10,r30
	ctx.r8.u64 = ctx.r10.u64 + ctx.r30.u64;
	// stfd f0,0(r7)
	REX_STORE_U64(ctx.r7.u32 + 0, ctx.f0.u64);
	// lfd f0,0(r10)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r10.u32 + 0);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stfd f0,0(r9)
	REX_STORE_U64(ctx.r9.u32 + 0, ctx.f0.u64);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// lfd f0,0(r8)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r8.u32 + 0);
	// add r8,r10,r30
	ctx.r8.u64 = ctx.r10.u64 + ctx.r30.u64;
	// stfd f0,0(r10)
	REX_STORE_U64(ctx.r10.u32 + 0, ctx.f0.u64);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lfd f0,0(r9)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r9.u32 + 0);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stfd f0,0(r8)
	REX_STORE_U64(ctx.r8.u32 + 0, ctx.f0.u64);
	// lfd f0,0(r10)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r10.u32 + 0);
	// stfd f0,0(r9)
	REX_STORE_U64(ctx.r9.u32 + 0, ctx.f0.u64);
	// lfdx f0,r10,r30
	ctx.f0.u64 = REX_LOAD_U64(ctx.r10.u32 + ctx.r30.u32);
	// stfdx f0,r9,r30
	REX_STORE_U64(ctx.r9.u32 + ctx.r30.u32, ctx.f0.u64);
	// b 0x829590a0
	goto loc_829590A0;
loc_8295837C:
	// clrlwi r7,r7,30
	ctx.r7.u64 = ctx.r7.u32 & 0x3;
	// cmplwi cr6,r7,0
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 0, ctx.xer);
	// add r7,r10,r30
	ctx.r7.u64 = ctx.r10.u64 + ctx.r30.u64;
	// bne cr6,0x8295843c
	if (!ctx.cr6.eq) goto loc_8295843C;
	// lwz r6,0(r8)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// stw r6,0(r26)
	REX_STORE_U32(ctx.r26.u32 + 0, ctx.r6.u32);
	// lwz r6,4(r8)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r8.u32 + 4);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stw r6,4(r26)
	REX_STORE_U32(ctx.r26.u32 + 4, ctx.r6.u32);
	// lwz r6,0(r9)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r9.u32 + 0);
	// stw r6,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r6.u32);
	// lwz r9,4(r9)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r9.u32 + 4);
	// stw r9,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r9.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// lwz r6,0(r8)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r8.u32 + 0);
	// add r10,r7,r30
	ctx.r10.u64 = ctx.r7.u64 + ctx.r30.u64;
	// stw r6,0(r7)
	REX_STORE_U32(ctx.r7.u32 + 0, ctx.r6.u32);
	// lwz r8,4(r8)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r8.u32 + 4);
	// stw r8,4(r7)
	REX_STORE_U32(ctx.r7.u32 + 4, ctx.r8.u32);
	// add r7,r9,r30
	ctx.r7.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lwz r6,0(r9)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r9.u32 + 0);
	// add r8,r10,r30
	ctx.r8.u64 = ctx.r10.u64 + ctx.r30.u64;
	// stw r6,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r6.u32);
	// lwz r9,4(r9)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r9.u32 + 4);
	// stw r9,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r9.u32);
	// add r9,r7,r30
	ctx.r9.u64 = ctx.r7.u64 + ctx.r30.u64;
	// lwz r6,0(r7)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r7.u32 + 0);
	// add r10,r8,r30
	ctx.r10.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r6,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r6.u32);
	// lwz r7,4(r7)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r7.u32 + 4);
	// stw r7,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r7.u32);
	// add r7,r9,r30
	ctx.r7.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lwz r6,0(r9)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r9.u32 + 0);
	// add r8,r10,r30
	ctx.r8.u64 = ctx.r10.u64 + ctx.r30.u64;
	// stw r6,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r6.u32);
	// lwz r9,4(r9)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r9.u32 + 4);
	// stw r9,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r9.u32);
	// add r9,r7,r30
	ctx.r9.u64 = ctx.r7.u64 + ctx.r30.u64;
	// lwz r6,0(r7)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r7.u32 + 0);
	// add r10,r8,r30
	ctx.r10.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r6,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r6.u32);
	// lwz r7,4(r7)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r7.u32 + 4);
	// stw r7,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r7.u32);
	// lwz r8,0(r9)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r9.u32 + 0);
	// stw r8,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r8.u32);
	// lwz r9,4(r9)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r9.u32 + 4);
	// stw r9,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r9.u32);
	// b 0x829590a0
	goto loc_829590A0;
loc_8295843C:
	// lbz r6,0(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 0);
	// stb r6,0(r26)
	REX_STORE_U8(ctx.r26.u32 + 0, ctx.r6.u8);
	// lbz r6,1(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 1);
	// stb r6,1(r26)
	REX_STORE_U8(ctx.r26.u32 + 1, ctx.r6.u8);
	// lbz r6,2(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 2);
	// stb r6,2(r26)
	REX_STORE_U8(ctx.r26.u32 + 2, ctx.r6.u8);
	// lbz r6,3(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 3);
	// stb r6,3(r26)
	REX_STORE_U8(ctx.r26.u32 + 3, ctx.r6.u8);
	// lbz r6,4(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 4);
	// stb r6,4(r26)
	REX_STORE_U8(ctx.r26.u32 + 4, ctx.r6.u8);
	// lbz r6,5(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 5);
	// stb r6,5(r26)
	REX_STORE_U8(ctx.r26.u32 + 5, ctx.r6.u8);
	// lbz r6,6(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 6);
	// stb r6,6(r26)
	REX_STORE_U8(ctx.r26.u32 + 6, ctx.r6.u8);
	// lbz r6,7(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 7);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stb r6,7(r26)
	REX_STORE_U8(ctx.r26.u32 + 7, ctx.r6.u8);
	// lbz r6,0(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 0);
	// stb r6,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r6.u8);
	// lbz r6,1(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 1);
	// stb r6,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r6.u8);
	// lbz r6,2(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 2);
	// stb r6,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r6.u8);
	// lbz r6,3(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 3);
	// stb r6,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r6.u8);
	// lbz r6,4(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 4);
	// stb r6,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r6.u8);
	// lbz r6,5(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 5);
	// stb r6,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r6.u8);
	// lbz r6,6(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 6);
	// stb r6,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r6.u8);
	// lbz r6,7(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 7);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stb r6,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r6.u8);
	// add r10,r7,r30
	ctx.r10.u64 = ctx.r7.u64 + ctx.r30.u64;
	// lbz r6,0(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 0);
	// stb r6,0(r7)
	REX_STORE_U8(ctx.r7.u32 + 0, ctx.r6.u8);
	// lbz r6,1(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 1);
	// stb r6,1(r7)
	REX_STORE_U8(ctx.r7.u32 + 1, ctx.r6.u8);
	// lbz r6,2(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 2);
	// stb r6,2(r7)
	REX_STORE_U8(ctx.r7.u32 + 2, ctx.r6.u8);
	// lbz r6,3(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 3);
	// stb r6,3(r7)
	REX_STORE_U8(ctx.r7.u32 + 3, ctx.r6.u8);
	// lbz r6,4(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 4);
	// stb r6,4(r7)
	REX_STORE_U8(ctx.r7.u32 + 4, ctx.r6.u8);
	// lbz r6,5(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 5);
	// stb r6,5(r7)
	REX_STORE_U8(ctx.r7.u32 + 5, ctx.r6.u8);
	// lbz r6,6(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 6);
	// stb r6,6(r7)
	REX_STORE_U8(ctx.r7.u32 + 6, ctx.r6.u8);
	// lbz r6,7(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 7);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stb r6,7(r7)
	REX_STORE_U8(ctx.r7.u32 + 7, ctx.r6.u8);
	// add r7,r10,r30
	ctx.r7.u64 = ctx.r10.u64 + ctx.r30.u64;
	// lbz r6,0(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 0);
	// stb r6,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r6.u8);
	// lbz r6,1(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 1);
	// stb r6,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r6.u8);
	// lbz r6,2(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 2);
	// stb r6,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r6.u8);
	// lbz r6,3(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 3);
	// stb r6,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r6.u8);
	// lbz r6,4(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 4);
	// stb r6,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r6.u8);
	// lbz r6,5(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 5);
	// stb r6,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r6.u8);
	// lbz r6,6(r9)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r9.u32 + 6);
	// stb r6,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r6.u8);
	// lbz r9,7(r9)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r9.u32 + 7);
	// stb r9,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r9.u8);
	// add r10,r8,r30
	ctx.r10.u64 = ctx.r8.u64 + ctx.r30.u64;
	// lbz r6,0(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 0);
	// add r9,r7,r30
	ctx.r9.u64 = ctx.r7.u64 + ctx.r30.u64;
	// stb r6,0(r7)
	REX_STORE_U8(ctx.r7.u32 + 0, ctx.r6.u8);
	// lbz r6,1(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 1);
	// stb r6,1(r7)
	REX_STORE_U8(ctx.r7.u32 + 1, ctx.r6.u8);
	// lbz r6,2(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 2);
	// stb r6,2(r7)
	REX_STORE_U8(ctx.r7.u32 + 2, ctx.r6.u8);
	// lbz r6,3(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 3);
	// stb r6,3(r7)
	REX_STORE_U8(ctx.r7.u32 + 3, ctx.r6.u8);
	// lbz r6,4(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 4);
	// stb r6,4(r7)
	REX_STORE_U8(ctx.r7.u32 + 4, ctx.r6.u8);
	// lbz r6,5(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 5);
	// stb r6,5(r7)
	REX_STORE_U8(ctx.r7.u32 + 5, ctx.r6.u8);
	// lbz r6,6(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 6);
	// stb r6,6(r7)
	REX_STORE_U8(ctx.r7.u32 + 6, ctx.r6.u8);
	// lbz r6,7(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 7);
	// add r8,r10,r30
	ctx.r8.u64 = ctx.r10.u64 + ctx.r30.u64;
	// stb r6,7(r7)
	REX_STORE_U8(ctx.r7.u32 + 7, ctx.r6.u8);
	// add r7,r9,r30
	ctx.r7.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lbz r6,0(r10)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r10.u32 + 0);
	// stb r6,0(r9)
	REX_STORE_U8(ctx.r9.u32 + 0, ctx.r6.u8);
	// lbz r6,1(r10)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r10.u32 + 1);
	// stb r6,1(r9)
	REX_STORE_U8(ctx.r9.u32 + 1, ctx.r6.u8);
	// lbz r6,2(r10)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r10.u32 + 2);
	// stb r6,2(r9)
	REX_STORE_U8(ctx.r9.u32 + 2, ctx.r6.u8);
	// lbz r6,3(r10)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r10.u32 + 3);
	// stb r6,3(r9)
	REX_STORE_U8(ctx.r9.u32 + 3, ctx.r6.u8);
	// lbz r6,4(r10)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r10.u32 + 4);
	// stb r6,4(r9)
	REX_STORE_U8(ctx.r9.u32 + 4, ctx.r6.u8);
	// lbz r6,5(r10)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r10.u32 + 5);
	// stb r6,5(r9)
	REX_STORE_U8(ctx.r9.u32 + 5, ctx.r6.u8);
	// lbz r6,6(r10)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r10.u32 + 6);
	// stb r6,6(r9)
	REX_STORE_U8(ctx.r9.u32 + 6, ctx.r6.u8);
	// lbz r6,7(r10)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r10.u32 + 7);
	// add r10,r8,r30
	ctx.r10.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stb r6,7(r9)
	REX_STORE_U8(ctx.r9.u32 + 7, ctx.r6.u8);
	// add r9,r7,r30
	ctx.r9.u64 = ctx.r7.u64 + ctx.r30.u64;
	// lbz r6,0(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 0);
	// stb r6,0(r7)
	REX_STORE_U8(ctx.r7.u32 + 0, ctx.r6.u8);
	// lbz r6,1(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 1);
	// stb r6,1(r7)
	REX_STORE_U8(ctx.r7.u32 + 1, ctx.r6.u8);
	// lbz r6,2(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 2);
	// stb r6,2(r7)
	REX_STORE_U8(ctx.r7.u32 + 2, ctx.r6.u8);
	// lbz r6,3(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 3);
	// stb r6,3(r7)
	REX_STORE_U8(ctx.r7.u32 + 3, ctx.r6.u8);
	// lbz r6,4(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 4);
	// stb r6,4(r7)
	REX_STORE_U8(ctx.r7.u32 + 4, ctx.r6.u8);
	// lbz r6,5(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 5);
	// stb r6,5(r7)
	REX_STORE_U8(ctx.r7.u32 + 5, ctx.r6.u8);
	// lbz r6,6(r8)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r8.u32 + 6);
	// stb r6,6(r7)
	REX_STORE_U8(ctx.r7.u32 + 6, ctx.r6.u8);
	// lbz r8,7(r8)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r8.u32 + 7);
	// stb r8,7(r7)
	REX_STORE_U8(ctx.r7.u32 + 7, ctx.r8.u8);
	// lbz r8,0(r10)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r10.u32 + 0);
	// stb r8,0(r9)
	REX_STORE_U8(ctx.r9.u32 + 0, ctx.r8.u8);
	// lbz r8,1(r10)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r10.u32 + 1);
	// stb r8,1(r9)
	REX_STORE_U8(ctx.r9.u32 + 1, ctx.r8.u8);
	// lbz r8,2(r10)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r10.u32 + 2);
	// stb r8,2(r9)
	REX_STORE_U8(ctx.r9.u32 + 2, ctx.r8.u8);
	// lbz r8,3(r10)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r10.u32 + 3);
	// stb r8,3(r9)
	REX_STORE_U8(ctx.r9.u32 + 3, ctx.r8.u8);
	// lbz r8,4(r10)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r10.u32 + 4);
	// stb r8,4(r9)
	REX_STORE_U8(ctx.r9.u32 + 4, ctx.r8.u8);
	// lbz r8,5(r10)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r10.u32 + 5);
	// stb r8,5(r9)
	REX_STORE_U8(ctx.r9.u32 + 5, ctx.r8.u8);
	// lbz r8,6(r10)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r10.u32 + 6);
	// stb r8,6(r9)
	REX_STORE_U8(ctx.r9.u32 + 6, ctx.r8.u8);
	// lbz r10,7(r10)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r10.u32 + 7);
	// stb r10,7(r9)
	REX_STORE_U8(ctx.r9.u32 + 7, ctx.r10.u8);
	// b 0x829590a0
	goto loc_829590A0;
loc_8295866C:
	// mr r4,r3
	ctx.r4.u64 = ctx.r3.u64;
	// cmplwi cr6,r25,4
	ctx.cr6.compare<uint32_t>(ctx.r25.u32, 4, ctx.xer);
	// bge cr6,0x8295869c
	if (!ctx.cr6.lt) goto loc_8295869C;
	// lwz r10,0(r3)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);
	// subfic r8,r25,4
	ctx.xer.ca = ctx.r25.u32 <= 4;
	ctx.r8.u64 = static_cast<uint64_t>(4) - ctx.r25.u64;
	// addi r5,r25,28
	ctx.r5.s64 = ctx.r25.s64 + 28;
	// addi r4,r3,4
	ctx.r4.s64 = ctx.r3.s64 + 4;
	// slw r9,r10,r25
	ctx.r9.u64 = ctx.r25.u8 & 0x20 ? 0 : (ctx.r10.u32 << (ctx.r25.u8 & 0x3F));
	// srw r6,r10,r8
	ctx.r6.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r8.u8 & 0x3F));
	// or r9,r9,r29
	ctx.r9.u64 = ctx.r9.u64 | ctx.r29.u64;
	// clrlwi r9,r9,28
	ctx.r9.u64 = ctx.r9.u32 & 0xF;
	// b 0x829586a8
	goto loc_829586A8;
loc_8295869C:
	// clrlwi r9,r29,28
	ctx.r9.u64 = ctx.r29.u32 & 0xF;
	// rlwinm r6,r29,28,4,31
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 28) & 0xFFFFFFF;
	// addi r5,r25,-4
	ctx.r5.s64 = ctx.r25.s64 + -4;
loc_829586A8:
	// rlwinm r8,r24,31,1,31
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 31) & 0x7FFFFFFF;
	// lwz r7,124(r1)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r1.u32 + 124);
	// rlwinm r9,r9,6,0,25
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 6) & 0xFFFFFFC0;
	// mr r10,r18
	ctx.r10.u64 = ctx.r18.u64;
	// add r7,r9,r7
	ctx.r7.u64 = ctx.r9.u64 + ctx.r7.u64;
	// stbx r28,r8,r23
	REX_STORE_U8(ctx.r8.u32 + ctx.r23.u32, ctx.r28.u8);
loc_829586C0:
	// cmplwi cr6,r5,0
	ctx.cr6.compare<uint32_t>(ctx.r5.u32, 0, ctx.xer);
	// bne cr6,0x829586dc
	if (!ctx.cr6.eq) goto loc_829586DC;
	// lwz r9,0(r4)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r4.u32 + 0);
	// li r5,31
	ctx.r5.s64 = 31;
	// addi r4,r4,4
	ctx.r4.s64 = ctx.r4.s64 + 4;
	// rlwinm r6,r9,31,1,31
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x829586e8
	goto loc_829586E8;
loc_829586DC:
	// mr r9,r6
	ctx.r9.u64 = ctx.r6.u64;
	// rlwinm r6,r6,31,1,31
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r5,r5,-1
	ctx.r5.s64 = ctx.r5.s64 + -1;
loc_829586E8:
	// clrlwi r9,r9,31
	ctx.r9.u64 = ctx.r9.u32 & 0x1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// beq cr6,0x82958734
	if (ctx.cr6.eq) goto loc_82958734;
	// lbz r8,0(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// lbz r9,0(r19)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r19.u32 + 0);
	// addi r19,r19,1
	ctx.r19.s64 = ctx.r19.s64 + 1;
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// cmpwi cr6,r9,0
	ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
	// beq cr6,0x82958770
	if (ctx.cr6.eq) goto loc_82958770;
loc_82958714:
	// lbzx r3,r10,r7
	ctx.r3.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r7.u32);
	// addi r29,r1,528
	ctx.r29.s64 = ctx.r1.s64 + 528;
	// addi r9,r9,-1
	ctx.r9.s64 = ctx.r9.s64 + -1;
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
	// cmpwi cr6,r9,0
	ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
	// stbx r8,r3,r29
	REX_STORE_U8(ctx.r3.u32 + ctx.r29.u32, ctx.r8.u8);
	// bne cr6,0x82958714
	if (!ctx.cr6.eq) goto loc_82958714;
	// b 0x82958770
	goto loc_82958770;
loc_82958734:
	// lbz r9,0(r19)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r19.u32 + 0);
	// addi r19,r19,1
	ctx.r19.s64 = ctx.r19.s64 + 1;
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// cmpwi cr6,r9,0
	ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
	// beq cr6,0x82958770
	if (ctx.cr6.eq) goto loc_82958770;
loc_82958748:
	// lbz r3,0(r11)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// addi r29,r1,528
	ctx.r29.s64 = ctx.r1.s64 + 528;
	// lbzx r8,r10,r7
	ctx.r8.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r7.u32);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// addi r9,r9,-1
	ctx.r9.s64 = ctx.r9.s64 + -1;
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
	// cmpwi cr6,r9,0
	ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// stbx r3,r8,r29
	REX_STORE_U8(ctx.r8.u32 + ctx.r29.u32, ctx.r3.u8);
	// bne cr6,0x82958748
	if (!ctx.cr6.eq) goto loc_82958748;
loc_82958770:
	// cmpwi cr6,r10,63
	ctx.cr6.compare<int32_t>(ctx.r10.s32, 63, ctx.xer);
	// blt cr6,0x829586c0
	if (ctx.cr6.lt) goto loc_829586C0;
	// stw r19,288(r1)
	REX_STORE_U32(ctx.r1.u32 + 288, ctx.r19.u32);
	// bne cr6,0x82958798
	if (!ctx.cr6.eq) goto loc_82958798;
	// lbz r10,0(r11)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// addi r8,r1,528
	ctx.r8.s64 = ctx.r1.s64 + 528;
	// lbz r9,63(r7)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r7.u32 + 63);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// stbx r10,r9,r8
	REX_STORE_U8(ctx.r9.u32 + ctx.r8.u32, ctx.r10.u8);
loc_82958798:
	// add r10,r26,r30
	ctx.r10.u64 = ctx.r26.u64 + ctx.r30.u64;
	// lfd f0,528(r1)
	ctx.fpscr.disableFlushMode();
	ctx.f0.u64 = REX_LOAD_U64(ctx.r1.u32 + 528);
	// stfd f0,0(r26)
	REX_STORE_U64(ctx.r26.u32 + 0, ctx.f0.u64);
	// mr r3,r4
	ctx.r3.u64 = ctx.r4.u64;
	// add r9,r10,r30
	ctx.r9.u64 = ctx.r10.u64 + ctx.r30.u64;
	// lfd f0,536(r1)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r1.u32 + 536);
	// lfd f13,544(r1)
	ctx.f13.u64 = REX_LOAD_U64(ctx.r1.u32 + 544);
	// mr r29,r6
	ctx.r29.u64 = ctx.r6.u64;
	// lfd f12,552(r1)
	ctx.f12.u64 = REX_LOAD_U64(ctx.r1.u32 + 552);
	// mr r25,r5
	ctx.r25.u64 = ctx.r5.u64;
	// stfd f0,0(r10)
	REX_STORE_U64(ctx.r10.u32 + 0, ctx.f0.u64);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lfd f11,560(r1)
	ctx.f11.u64 = REX_LOAD_U64(ctx.r1.u32 + 560);
	// stw r3,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r3.u32);
	// stfd f13,0(r9)
	REX_STORE_U64(ctx.r9.u32 + 0, ctx.f13.u64);
	// add r9,r10,r30
	ctx.r9.u64 = ctx.r10.u64 + ctx.r30.u64;
	// lfd f10,568(r1)
	ctx.f10.u64 = REX_LOAD_U64(ctx.r1.u32 + 568);
	// stw r29,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r29.u32);
	// lfd f9,576(r1)
	ctx.f9.u64 = REX_LOAD_U64(ctx.r1.u32 + 576);
	// stw r25,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r25.u32);
	// stfd f12,0(r10)
	REX_STORE_U64(ctx.r10.u32 + 0, ctx.f12.u64);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lfd f8,584(r1)
	ctx.f8.u64 = REX_LOAD_U64(ctx.r1.u32 + 584);
	// stfd f11,0(r9)
	REX_STORE_U64(ctx.r9.u32 + 0, ctx.f11.u64);
	// add r9,r10,r30
	ctx.r9.u64 = ctx.r10.u64 + ctx.r30.u64;
	// stfd f10,0(r10)
	REX_STORE_U64(ctx.r10.u32 + 0, ctx.f10.u64);
	// stfd f9,0(r9)
	REX_STORE_U64(ctx.r9.u32 + 0, ctx.f9.u64);
	// stfdx f8,r9,r30
	REX_STORE_U64(ctx.r9.u32 + ctx.r30.u32, ctx.f8.u64);
	// b 0x829590a0
	goto loc_829590A0;
loc_8295880C:
	// rlwinm r11,r24,31,1,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 31) & 0x7FFFFFFF;
	// cmplwi cr6,r25,7
	ctx.cr6.compare<uint32_t>(ctx.r25.u32, 7, ctx.xer);
	// stbx r28,r11,r23
	REX_STORE_U8(ctx.r11.u32 + ctx.r23.u32, ctx.r28.u8);
	// lbz r11,0(r22)
	ctx.r11.u64 = REX_LOAD_U8(ctx.r22.u32 + 0);
	// addi r22,r22,1
	ctx.r22.s64 = ctx.r22.s64 + 1;
	// lbz r10,0(r21)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r21.u32 + 0);
	// addi r21,r21,1
	ctx.r21.s64 = ctx.r21.s64 + 1;
	// extsb r11,r11
	ctx.r11.s64 = ctx.r11.s8;
	// extsb r10,r10
	ctx.r10.s64 = ctx.r10.s8;
	// mullw r11,r11,r30
	ctx.r11.s64 = int64_t(ctx.r11.s32) * int64_t(ctx.r30.s32);
	// stw r22,192(r1)
	REX_STORE_U32(ctx.r1.u32 + 192, ctx.r22.u32);
	// stw r21,240(r1)
	REX_STORE_U32(ctx.r1.u32 + 240, ctx.r21.u32);
	// add r11,r11,r10
	ctx.r11.u64 = ctx.r11.u64 + ctx.r10.u64;
	// add r7,r11,r20
	ctx.r7.u64 = ctx.r11.u64 + ctx.r20.u64;
	// bge cr6,0x82958878
	if (!ctx.cr6.lt) goto loc_82958878;
	// lwz r11,0(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);
	// subfic r10,r25,7
	ctx.xer.ca = ctx.r25.u32 <= 7;
	ctx.r10.u64 = static_cast<uint64_t>(7) - ctx.r25.u64;
	// addi r9,r3,4
	ctx.r9.s64 = ctx.r3.s64 + 4;
	// addi r8,r25,25
	ctx.r8.s64 = ctx.r25.s64 + 25;
	// stw r9,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r9.u32);
	// stw r8,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r8.u32);
	// slw r9,r11,r25
	ctx.r9.u64 = ctx.r25.u8 & 0x20 ? 0 : (ctx.r11.u32 << (ctx.r25.u8 & 0x3F));
	// srw r11,r11,r10
	ctx.r11.u64 = ctx.r10.u8 & 0x20 ? 0 : (ctx.r11.u32 >> (ctx.r10.u8 & 0x3F));
	// or r9,r9,r29
	ctx.r9.u64 = ctx.r9.u64 | ctx.r29.u64;
	// clrlwi r6,r9,25
	ctx.r6.u64 = ctx.r9.u32 & 0x7F;
	// stw r11,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r11.u32);
	// b 0x8295888c
	goto loc_8295888C;
loc_82958878:
	// rlwinm r11,r29,25,7,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 25) & 0x1FFFFFF;
	// clrlwi r6,r29,25
	ctx.r6.u64 = ctx.r29.u32 & 0x7F;
	// stw r11,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r11.u32);
	// addi r11,r25,-7
	ctx.r11.s64 = ctx.r25.s64 + -7;
	// stw r11,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r11.u32);
loc_8295888C:
	// mr r8,r30
	ctx.r8.u64 = ctx.r30.u64;
	// addi r5,r1,80
	ctx.r5.s64 = ctx.r1.s64 + 80;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r26
	ctx.r3.u64 = ctx.r26.u64;
	// bl 0x829600c0
	ctx.lr = 0x829588A0;
	sub_829600C0(ctx, base);
	// lwz r25,88(r1)
	ctx.r25.u64 = REX_LOAD_U32(ctx.r1.u32 + 88);
	// lwz r3,84(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 84);
	// lwz r29,80(r1)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// lwz r11,128(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 128);
	// b 0x829590a0
	goto loc_829590A0;
loc_829588B4:
	// rlwinm r11,r24,31,1,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,640
	ctx.r3.s64 = ctx.r1.s64 + 640;
	// stbx r28,r11,r23
	REX_STORE_U8(ctx.r11.u32 + ctx.r23.u32, ctx.r28.u8);
	// lhz r11,0(r16)
	ctx.r11.u64 = REX_LOAD_U16(ctx.r16.u32 + 0);
	// addi r16,r16,2
	ctx.r16.s64 = ctx.r16.s64 + 2;
	// sth r11,640(r1)
	REX_STORE_U16(ctx.r1.u32 + 640, ctx.r11.u16);
	// stw r16,384(r1)
	REX_STORE_U32(ctx.r1.u32 + 384, ctx.r16.u32);
	// bl 0x8295f208
	ctx.lr = 0x829588D8;
	sub_8295F208(ctx, base);
	// lwz r11,88(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 88);
	// cmplwi cr6,r11,4
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 4, ctx.xer);
	// bge cr6,0x8295891c
	if (!ctx.cr6.lt) goto loc_8295891C;
	// lwz r9,84(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 84);
	// subfic r8,r11,4
	ctx.xer.ca = ctx.r11.u32 <= 4;
	ctx.r8.u64 = static_cast<uint64_t>(4) - ctx.r11.u64;
	// lwz r10,0(r9)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r9.u32 + 0);
	// addi r9,r9,4
	ctx.r9.s64 = ctx.r9.s64 + 4;
	// stw r9,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r9.u32);
	// slw r9,r10,r11
	ctx.r9.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r10.u32 << (ctx.r11.u8 & 0x3F));
	// addi r11,r11,28
	ctx.r11.s64 = ctx.r11.s64 + 28;
	// stw r11,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r11.u32);
	// lwz r11,80(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// or r11,r9,r11
	ctx.r11.u64 = ctx.r9.u64 | ctx.r11.u64;
	// clrlwi r6,r11,28
	ctx.r6.u64 = ctx.r11.u32 & 0xF;
	// srw r11,r10,r8
	ctx.r11.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r8.u8 & 0x3F));
	// stw r11,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r11.u32);
	// b 0x82958934
	goto loc_82958934;
loc_8295891C:
	// lwz r10,80(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// addi r11,r11,-4
	ctx.r11.s64 = ctx.r11.s64 + -4;
	// clrlwi r6,r10,28
	ctx.r6.u64 = ctx.r10.u32 & 0xF;
	// rlwinm r10,r10,28,4,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 28) & 0xFFFFFFF;
	// stw r11,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r11.u32);
	// stw r10,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r10.u32);
loc_82958934:
	// addi r5,r1,640
	ctx.r5.s64 = ctx.r1.s64 + 640;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r26
	ctx.r3.u64 = ctx.r26.u64;
	// bl 0x8295ee50
	ctx.lr = 0x82958944;
	sub_8295EE50(ctx, base);
	// lwz r25,88(r1)
	ctx.r25.u64 = REX_LOAD_U32(ctx.r1.u32 + 88);
	// lwz r3,84(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 84);
	// lwz r29,80(r1)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// lwz r11,128(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 128);
	// b 0x829590a0
	goto loc_829590A0;
loc_82958958:
	// lbz r10,0(r11)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// rlwinm r8,r24,31,1,31
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 31) & 0x7FFFFFFF;
	// add r9,r26,r30
	ctx.r9.u64 = ctx.r26.u64 + ctx.r30.u64;
	// rotlwi r7,r10,8
	ctx.r7.u64 = __builtin_rotateleft32(ctx.r10.u32, 8);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// or r10,r7,r10
	ctx.r10.u64 = ctx.r7.u64 | ctx.r10.u64;
	// stbx r28,r8,r23
	REX_STORE_U8(ctx.r8.u32 + ctx.r23.u32, ctx.r28.u8);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// rlwinm r7,r10,16,0,15
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 16) & 0xFFFF0000;
	// or r10,r7,r10
	ctx.r10.u64 = ctx.r7.u64 | ctx.r10.u64;
	// stw r10,0(r26)
	REX_STORE_U32(ctx.r26.u32 + 0, ctx.r10.u32);
	// stw r10,4(r26)
	REX_STORE_U32(ctx.r26.u32 + 4, ctx.r10.u32);
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r10,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r10.u32);
	// stw r10,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r10.u32);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r10,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r10.u32);
	// stw r10,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r10.u32);
	// add r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 + ctx.r30.u64;
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// add r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stw r10,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r10.u32);
	// stw r10,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r10.u32);
	// stw r10,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r10.u32);
	// stw r10,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r10.u32);
	// b 0x8295909c
	goto loc_8295909C;
loc_829589D8:
	// rlwinm r11,r24,31,1,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 31) & 0x7FFFFFFF;
	// lwz r9,592(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 592);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// addi r3,r1,640
	ctx.r3.s64 = ctx.r1.s64 + 640;
	// stbx r28,r11,r23
	REX_STORE_U8(ctx.r11.u32 + ctx.r23.u32, ctx.r28.u8);
	// lbz r11,0(r22)
	ctx.r11.u64 = REX_LOAD_U8(ctx.r22.u32 + 0);
	// addi r22,r22,1
	ctx.r22.s64 = ctx.r22.s64 + 1;
	// lbz r10,0(r21)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r21.u32 + 0);
	// addi r21,r21,1
	ctx.r21.s64 = ctx.r21.s64 + 1;
	// extsb r11,r11
	ctx.r11.s64 = ctx.r11.s8;
	// lhz r8,0(r9)
	ctx.r8.u64 = REX_LOAD_U16(ctx.r9.u32 + 0);
	// extsb r10,r10
	ctx.r10.s64 = ctx.r10.s8;
	// mullw r11,r11,r30
	ctx.r11.s64 = int64_t(ctx.r11.s32) * int64_t(ctx.r30.s32);
	// stw r22,192(r1)
	REX_STORE_U32(ctx.r1.u32 + 192, ctx.r22.u32);
	// stw r21,240(r1)
	REX_STORE_U32(ctx.r1.u32 + 240, ctx.r21.u32);
	// sth r8,640(r1)
	REX_STORE_U16(ctx.r1.u32 + 640, ctx.r8.u16);
	// add r11,r11,r10
	ctx.r11.u64 = ctx.r11.u64 + ctx.r10.u64;
	// addi r10,r9,2
	ctx.r10.s64 = ctx.r9.s64 + 2;
	// add r29,r11,r20
	ctx.r29.u64 = ctx.r11.u64 + ctx.r20.u64;
	// stw r10,592(r1)
	REX_STORE_U32(ctx.r1.u32 + 592, ctx.r10.u32);
	// bl 0x8295f208
	ctx.lr = 0x82958A2C;
	sub_8295F208(ctx, base);
	// lwz r11,88(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 88);
	// cmplwi cr6,r11,4
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 4, ctx.xer);
	// bge cr6,0x82958a70
	if (!ctx.cr6.lt) goto loc_82958A70;
	// lwz r9,84(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 84);
	// subfic r8,r11,4
	ctx.xer.ca = ctx.r11.u32 <= 4;
	ctx.r8.u64 = static_cast<uint64_t>(4) - ctx.r11.u64;
	// lwz r10,0(r9)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r9.u32 + 0);
	// addi r9,r9,4
	ctx.r9.s64 = ctx.r9.s64 + 4;
	// stw r9,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r9.u32);
	// slw r9,r10,r11
	ctx.r9.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r10.u32 << (ctx.r11.u8 & 0x3F));
	// addi r11,r11,28
	ctx.r11.s64 = ctx.r11.s64 + 28;
	// stw r11,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r11.u32);
	// lwz r11,80(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// or r11,r9,r11
	ctx.r11.u64 = ctx.r9.u64 | ctx.r11.u64;
	// clrlwi r6,r11,28
	ctx.r6.u64 = ctx.r11.u32 & 0xF;
	// srw r11,r10,r8
	ctx.r11.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r8.u8 & 0x3F));
	// stw r11,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r11.u32);
	// b 0x82958a88
	goto loc_82958A88;
loc_82958A70:
	// lwz r10,80(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// addi r11,r11,-4
	ctx.r11.s64 = ctx.r11.s64 + -4;
	// clrlwi r6,r10,28
	ctx.r6.u64 = ctx.r10.u32 & 0xF;
	// rlwinm r10,r10,28,4,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 28) & 0xFFFFFFF;
	// stw r11,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r11.u32);
	// stw r10,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r10.u32);
loc_82958A88:
	// mr r8,r30
	ctx.r8.u64 = ctx.r30.u64;
	// mr r7,r29
	ctx.r7.u64 = ctx.r29.u64;
	// addi r5,r1,640
	ctx.r5.s64 = ctx.r1.s64 + 640;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r26
	ctx.r3.u64 = ctx.r26.u64;
	// bl 0x8295ee80
	ctx.lr = 0x82958AA0;
	sub_8295EE80(ctx, base);
	// lwz r25,88(r1)
	ctx.r25.u64 = REX_LOAD_U32(ctx.r1.u32 + 88);
	// lwz r3,84(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 84);
	// lwz r29,80(r1)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// lwz r11,128(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 128);
	// b 0x829590a0
	goto loc_829590A0;
loc_82958AB4:
	// lbz r10,0(r11)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// rlwinm r8,r24,31,1,31
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 31) & 0x7FFFFFFF;
	// rotlwi r5,r10,8
	ctx.r5.u64 = __builtin_rotateleft32(ctx.r10.u32, 8);
	// addi r19,r31,64
	ctx.r19.s64 = ctx.r31.s64 + 64;
	// or r10,r5,r10
	ctx.r10.u64 = ctx.r5.u64 | ctx.r10.u64;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// addi r18,r31,64
	ctx.r18.s64 = ctx.r31.s64 + 64;
	// lbz r9,0(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// addi r11,r11,1
	ctx.r11.s64 = ctx.r11.s64 + 1;
	// stbx r28,r8,r23
	REX_STORE_U8(ctx.r8.u32 + ctx.r23.u32, ctx.r28.u8);
	// rlwinm r15,r10,16,0,15
	ctx.r15.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 16) & 0xFFFF0000;
	// rotlwi r4,r9,8
	ctx.r4.u64 = __builtin_rotateleft32(ctx.r9.u32, 8);
	// or r10,r15,r10
	ctx.r10.u64 = ctx.r15.u64 | ctx.r10.u64;
	// or r9,r4,r9
	ctx.r9.u64 = ctx.r4.u64 | ctx.r9.u64;
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
	// addi r6,r27,1
	ctx.r6.s64 = ctx.r27.s64 + 1;
	// lbz r7,0(r27)
	ctx.r7.u64 = REX_LOAD_U8(ctx.r27.u32 + 0);
	// rlwinm r14,r9,16,0,15
	ctx.r14.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 16) & 0xFFFF0000;
	// addi r27,r31,64
	ctx.r27.s64 = ctx.r31.s64 + 64;
	// rlwinm r5,r7,2,26,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0x3C;
	// or r9,r14,r9
	ctx.r9.u64 = ctx.r14.u64 | ctx.r9.u64;
	// rlwinm r4,r7,30,2,29
	ctx.r4.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 30) & 0x3FFFFFFC;
	// add r8,r26,r30
	ctx.r8.u64 = ctx.r26.u64 + ctx.r30.u64;
	// addi r17,r31,64
	ctx.r17.s64 = ctx.r31.s64 + 64;
	// lwzx r15,r5,r31
	ctx.r15.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r31.u32);
	// addi r16,r31,64
	ctx.r16.s64 = ctx.r31.s64 + 64;
	// lwzx r5,r5,r19
	ctx.r5.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r19.u32);
	// add r7,r8,r30
	ctx.r7.u64 = ctx.r8.u64 + ctx.r30.u64;
	// and r19,r15,r9
	ctx.r19.u64 = ctx.r15.u64 & ctx.r9.u64;
	// and r5,r5,r10
	ctx.r5.u64 = ctx.r5.u64 & ctx.r10.u64;
	// or r5,r19,r5
	ctx.r5.u64 = ctx.r19.u64 | ctx.r5.u64;
	// stw r5,0(r26)
	REX_STORE_U32(ctx.r26.u32 + 0, ctx.r5.u32);
	// lwzx r5,r4,r31
	ctx.r5.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r31.u32);
	// lwzx r4,r4,r18
	ctx.r4.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r18.u32);
	// addi r18,r31,64
	ctx.r18.s64 = ctx.r31.s64 + 64;
	// and r5,r5,r9
	ctx.r5.u64 = ctx.r5.u64 & ctx.r9.u64;
	// and r4,r4,r10
	ctx.r4.u64 = ctx.r4.u64 & ctx.r10.u64;
	// or r5,r5,r4
	ctx.r5.u64 = ctx.r5.u64 | ctx.r4.u64;
	// stw r5,4(r26)
	REX_STORE_U32(ctx.r26.u32 + 4, ctx.r5.u32);
	// lbz r5,0(r6)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r6.u32 + 0);
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
	// rlwinm r4,r5,2,26,29
	ctx.r4.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 2) & 0x3C;
	// rlwinm r5,r5,30,2,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 30) & 0x3FFFFFFC;
	// lwzx r19,r4,r31
	ctx.r19.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r31.u32);
	// lwzx r4,r4,r27
	ctx.r4.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r27.u32);
	// and r27,r19,r9
	ctx.r27.u64 = ctx.r19.u64 & ctx.r9.u64;
	// and r4,r4,r10
	ctx.r4.u64 = ctx.r4.u64 & ctx.r10.u64;
	// or r4,r27,r4
	ctx.r4.u64 = ctx.r27.u64 | ctx.r4.u64;
	// stw r4,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r4.u32);
	// lwzx r4,r5,r31
	ctx.r4.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r31.u32);
	// lwzx r5,r5,r17
	ctx.r5.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r17.u32);
	// addi r17,r31,64
	ctx.r17.s64 = ctx.r31.s64 + 64;
	// and r4,r4,r9
	ctx.r4.u64 = ctx.r4.u64 & ctx.r9.u64;
	// and r5,r5,r10
	ctx.r5.u64 = ctx.r5.u64 & ctx.r10.u64;
	// or r5,r4,r5
	ctx.r5.u64 = ctx.r4.u64 | ctx.r5.u64;
	// stw r5,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r5.u32);
	// add r8,r7,r30
	ctx.r8.u64 = ctx.r7.u64 + ctx.r30.u64;
	// lbz r5,0(r6)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r6.u32 + 0);
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
	// rlwinm r4,r5,2,26,29
	ctx.r4.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 2) & 0x3C;
	// rlwinm r5,r5,30,2,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 30) & 0x3FFFFFFC;
	// lwzx r27,r4,r31
	ctx.r27.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r31.u32);
	// lwzx r4,r4,r16
	ctx.r4.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r16.u32);
	// addi r16,r31,64
	ctx.r16.s64 = ctx.r31.s64 + 64;
	// and r27,r27,r9
	ctx.r27.u64 = ctx.r27.u64 & ctx.r9.u64;
	// and r4,r4,r10
	ctx.r4.u64 = ctx.r4.u64 & ctx.r10.u64;
	// or r4,r27,r4
	ctx.r4.u64 = ctx.r27.u64 | ctx.r4.u64;
	// addi r27,r31,64
	ctx.r27.s64 = ctx.r31.s64 + 64;
	// stw r4,0(r7)
	REX_STORE_U32(ctx.r7.u32 + 0, ctx.r4.u32);
	// addi r4,r31,64
	ctx.r4.s64 = ctx.r31.s64 + 64;
	// lwzx r19,r5,r31
	ctx.r19.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r31.u32);
	// lwzx r5,r5,r4
	ctx.r5.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r4.u32);
	// and r4,r19,r9
	ctx.r4.u64 = ctx.r19.u64 & ctx.r9.u64;
	// addi r19,r31,64
	ctx.r19.s64 = ctx.r31.s64 + 64;
	// and r5,r5,r10
	ctx.r5.u64 = ctx.r5.u64 & ctx.r10.u64;
	// or r5,r4,r5
	ctx.r5.u64 = ctx.r4.u64 | ctx.r5.u64;
	// stw r5,4(r7)
	REX_STORE_U32(ctx.r7.u32 + 4, ctx.r5.u32);
	// add r7,r8,r30
	ctx.r7.u64 = ctx.r8.u64 + ctx.r30.u64;
	// lbz r5,0(r6)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r6.u32 + 0);
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
	// rlwinm r4,r5,2,26,29
	ctx.r4.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 2) & 0x3C;
	// rlwinm r5,r5,30,2,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 30) & 0x3FFFFFFC;
	// addi r15,r31,64
	ctx.r15.s64 = ctx.r31.s64 + 64;
	// lwzx r14,r4,r31
	ctx.r14.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r31.u32);
	// lwzx r4,r4,r27
	ctx.r4.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r27.u32);
	// and r27,r14,r9
	ctx.r27.u64 = ctx.r14.u64 & ctx.r9.u64;
	// and r4,r4,r10
	ctx.r4.u64 = ctx.r4.u64 & ctx.r10.u64;
	// or r4,r27,r4
	ctx.r4.u64 = ctx.r27.u64 | ctx.r4.u64;
	// stw r4,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r4.u32);
	// lwzx r4,r5,r31
	ctx.r4.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r31.u32);
	// lwzx r5,r5,r19
	ctx.r5.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r19.u32);
	// addi r19,r31,64
	ctx.r19.s64 = ctx.r31.s64 + 64;
	// and r4,r4,r9
	ctx.r4.u64 = ctx.r4.u64 & ctx.r9.u64;
	// and r5,r5,r10
	ctx.r5.u64 = ctx.r5.u64 & ctx.r10.u64;
	// or r5,r4,r5
	ctx.r5.u64 = ctx.r4.u64 | ctx.r5.u64;
	// stw r5,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r5.u32);
	// add r8,r7,r30
	ctx.r8.u64 = ctx.r7.u64 + ctx.r30.u64;
	// lbz r5,0(r6)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r6.u32 + 0);
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
	// rlwinm r4,r5,2,26,29
	ctx.r4.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 2) & 0x3C;
	// rlwinm r5,r5,30,2,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 30) & 0x3FFFFFFC;
	// lwzx r27,r4,r31
	ctx.r27.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r31.u32);
	// lwzx r4,r4,r18
	ctx.r4.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r18.u32);
	// addi r18,r31,64
	ctx.r18.s64 = ctx.r31.s64 + 64;
	// and r27,r27,r9
	ctx.r27.u64 = ctx.r27.u64 & ctx.r9.u64;
	// and r4,r4,r10
	ctx.r4.u64 = ctx.r4.u64 & ctx.r10.u64;
	// or r4,r27,r4
	ctx.r4.u64 = ctx.r27.u64 | ctx.r4.u64;
	// stw r4,0(r7)
	REX_STORE_U32(ctx.r7.u32 + 0, ctx.r4.u32);
	// lwzx r4,r5,r31
	ctx.r4.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r31.u32);
	// lwzx r5,r5,r17
	ctx.r5.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r17.u32);
	// addi r17,r31,64
	ctx.r17.s64 = ctx.r31.s64 + 64;
	// and r4,r4,r9
	ctx.r4.u64 = ctx.r4.u64 & ctx.r9.u64;
	// and r5,r5,r10
	ctx.r5.u64 = ctx.r5.u64 & ctx.r10.u64;
	// or r5,r4,r5
	ctx.r5.u64 = ctx.r4.u64 | ctx.r5.u64;
	// stw r5,4(r7)
	REX_STORE_U32(ctx.r7.u32 + 4, ctx.r5.u32);
	// add r7,r8,r30
	ctx.r7.u64 = ctx.r8.u64 + ctx.r30.u64;
	// lbz r5,0(r6)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r6.u32 + 0);
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
	// rlwinm r4,r5,2,26,29
	ctx.r4.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 2) & 0x3C;
	// rlwinm r5,r5,30,2,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 30) & 0x3FFFFFFC;
	// lwzx r27,r4,r31
	ctx.r27.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r31.u32);
	// lwzx r4,r4,r16
	ctx.r4.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r16.u32);
	// and r27,r27,r9
	ctx.r27.u64 = ctx.r27.u64 & ctx.r9.u64;
	// and r4,r4,r10
	ctx.r4.u64 = ctx.r4.u64 & ctx.r10.u64;
	// or r4,r27,r4
	ctx.r4.u64 = ctx.r27.u64 | ctx.r4.u64;
	// addi r27,r31,64
	ctx.r27.s64 = ctx.r31.s64 + 64;
	// stw r4,0(r8)
	REX_STORE_U32(ctx.r8.u32 + 0, ctx.r4.u32);
	// lwzx r4,r5,r31
	ctx.r4.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r31.u32);
	// lwzx r5,r5,r15
	ctx.r5.u64 = REX_LOAD_U32(ctx.r5.u32 + ctx.r15.u32);
	// and r4,r4,r9
	ctx.r4.u64 = ctx.r4.u64 & ctx.r9.u64;
	// and r5,r5,r10
	ctx.r5.u64 = ctx.r5.u64 & ctx.r10.u64;
	// or r5,r4,r5
	ctx.r5.u64 = ctx.r4.u64 | ctx.r5.u64;
	// stw r5,4(r8)
	REX_STORE_U32(ctx.r8.u32 + 4, ctx.r5.u32);
	// add r5,r7,r30
	ctx.r5.u64 = ctx.r7.u64 + ctx.r30.u64;
	// lbz r8,0(r6)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r6.u32 + 0);
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
	// rlwinm r4,r8,2,26,29
	ctx.r4.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0x3C;
	// rlwinm r8,r8,30,2,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 30) & 0x3FFFFFFC;
	// lwzx r16,r4,r31
	ctx.r16.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r31.u32);
	// lwzx r4,r4,r27
	ctx.r4.u64 = REX_LOAD_U32(ctx.r4.u32 + ctx.r27.u32);
	// and r27,r16,r9
	ctx.r27.u64 = ctx.r16.u64 & ctx.r9.u64;
	// and r4,r4,r10
	ctx.r4.u64 = ctx.r4.u64 & ctx.r10.u64;
	// or r4,r27,r4
	ctx.r4.u64 = ctx.r27.u64 | ctx.r4.u64;
	// addi r27,r6,1
	ctx.r27.s64 = ctx.r6.s64 + 1;
	// stw r4,0(r7)
	REX_STORE_U32(ctx.r7.u32 + 0, ctx.r4.u32);
	// lwzx r4,r8,r31
	ctx.r4.u64 = REX_LOAD_U32(ctx.r8.u32 + ctx.r31.u32);
	// lwzx r8,r8,r19
	ctx.r8.u64 = REX_LOAD_U32(ctx.r8.u32 + ctx.r19.u32);
	// and r4,r4,r9
	ctx.r4.u64 = ctx.r4.u64 & ctx.r9.u64;
	// and r8,r8,r10
	ctx.r8.u64 = ctx.r8.u64 & ctx.r10.u64;
	// or r8,r4,r8
	ctx.r8.u64 = ctx.r4.u64 | ctx.r8.u64;
	// stw r8,4(r7)
	REX_STORE_U32(ctx.r7.u32 + 4, ctx.r8.u32);
	// lbz r8,0(r6)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r6.u32 + 0);
	// rlwinm r7,r8,2,26,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0x3C;
	// rlwinm r8,r8,30,2,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 30) & 0x3FFFFFFC;
	// lwzx r6,r7,r31
	ctx.r6.u64 = REX_LOAD_U32(ctx.r7.u32 + ctx.r31.u32);
	// lwzx r7,r7,r18
	ctx.r7.u64 = REX_LOAD_U32(ctx.r7.u32 + ctx.r18.u32);
	// and r6,r6,r9
	ctx.r6.u64 = ctx.r6.u64 & ctx.r9.u64;
	// and r7,r7,r10
	ctx.r7.u64 = ctx.r7.u64 & ctx.r10.u64;
	// or r7,r6,r7
	ctx.r7.u64 = ctx.r6.u64 | ctx.r7.u64;
	// stw r7,0(r5)
	REX_STORE_U32(ctx.r5.u32 + 0, ctx.r7.u32);
	// lwzx r7,r8,r31
	ctx.r7.u64 = REX_LOAD_U32(ctx.r8.u32 + ctx.r31.u32);
	// lwzx r8,r8,r17
	ctx.r8.u64 = REX_LOAD_U32(ctx.r8.u32 + ctx.r17.u32);
	// and r9,r7,r9
	ctx.r9.u64 = ctx.r7.u64 & ctx.r9.u64;
	// lwz r16,384(r1)
	ctx.r16.u64 = REX_LOAD_U32(ctx.r1.u32 + 384);
	// li r18,0
	ctx.r18.s64 = 0;
	// and r10,r8,r10
	ctx.r10.u64 = ctx.r8.u64 & ctx.r10.u64;
	// lwz r19,288(r1)
	ctx.r19.u64 = REX_LOAD_U32(ctx.r1.u32 + 288);
	// lwz r17,116(r1)
	ctx.r17.u64 = REX_LOAD_U32(ctx.r1.u32 + 116);
	// or r10,r9,r10
	ctx.r10.u64 = ctx.r9.u64 | ctx.r10.u64;
	// lwz r15,1404(r1)
	ctx.r15.u64 = REX_LOAD_U32(ctx.r1.u32 + 1404);
	// stw r10,4(r5)
	REX_STORE_U32(ctx.r5.u32 + 4, ctx.r10.u32);
	// b 0x829590a0
	goto loc_829590A0;
loc_82958D68:
	// rlwinm r8,r24,31,1,31
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 31) & 0x7FFFFFFF;
	// or r9,r11,r26
	ctx.r9.u64 = ctx.r11.u64 | ctx.r26.u64;
	// add r10,r26,r30
	ctx.r10.u64 = ctx.r26.u64 + ctx.r30.u64;
	// stbx r28,r8,r23
	REX_STORE_U8(ctx.r8.u32 + ctx.r23.u32, ctx.r28.u8);
	// clrlwi r8,r9,29
	ctx.r8.u64 = ctx.r9.u32 & 0x7;
	// cmplwi cr6,r8,0
	ctx.cr6.compare<uint32_t>(ctx.r8.u32, 0, ctx.xer);
	// bne cr6,0x82958ddc
	if (!ctx.cr6.eq) goto loc_82958DDC;
	// lfd f0,0(r11)
	ctx.fpscr.disableFlushMode();
	ctx.f0.u64 = REX_LOAD_U64(ctx.r11.u32 + 0);
	// add r9,r10,r30
	ctx.r9.u64 = ctx.r10.u64 + ctx.r30.u64;
	// stfd f0,0(r26)
	REX_STORE_U64(ctx.r26.u32 + 0, ctx.f0.u64);
	// lfd f0,8(r11)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r11.u32 + 8);
	// stfd f0,0(r10)
	REX_STORE_U64(ctx.r10.u32 + 0, ctx.f0.u64);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lfd f0,16(r11)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r11.u32 + 16);
	// stfd f0,0(r9)
	REX_STORE_U64(ctx.r9.u32 + 0, ctx.f0.u64);
	// add r9,r10,r30
	ctx.r9.u64 = ctx.r10.u64 + ctx.r30.u64;
	// lfd f0,24(r11)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r11.u32 + 24);
	// stfd f0,0(r10)
	REX_STORE_U64(ctx.r10.u32 + 0, ctx.f0.u64);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lfd f0,32(r11)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r11.u32 + 32);
	// stfd f0,0(r9)
	REX_STORE_U64(ctx.r9.u32 + 0, ctx.f0.u64);
	// add r9,r10,r30
	ctx.r9.u64 = ctx.r10.u64 + ctx.r30.u64;
	// lfd f0,40(r11)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r11.u32 + 40);
	// stfd f0,0(r10)
	REX_STORE_U64(ctx.r10.u32 + 0, ctx.f0.u64);
	// lfd f0,48(r11)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r11.u32 + 48);
	// stfd f0,0(r9)
	REX_STORE_U64(ctx.r9.u32 + 0, ctx.f0.u64);
	// lfd f0,56(r11)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r11.u32 + 56);
	// stfdx f0,r9,r30
	REX_STORE_U64(ctx.r9.u32 + ctx.r30.u32, ctx.f0.u64);
	// b 0x82959098
	goto loc_82959098;
loc_82958DDC:
	// clrlwi r9,r9,30
	ctx.r9.u64 = ctx.r9.u32 & 0x3;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// add r9,r10,r30
	ctx.r9.u64 = ctx.r10.u64 + ctx.r30.u64;
	// bne cr6,0x82958e84
	if (!ctx.cr6.eq) goto loc_82958E84;
	// lwz r8,0(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// stw r8,0(r26)
	REX_STORE_U32(ctx.r26.u32 + 0, ctx.r8.u32);
	// lwz r8,4(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 4);
	// stw r8,4(r26)
	REX_STORE_U32(ctx.r26.u32 + 4, ctx.r8.u32);
	// lwz r8,8(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 8);
	// stw r8,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r8.u32);
	// lwz r8,12(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 12);
	// stw r8,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r8.u32);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lwz r8,16(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 16);
	// stw r8,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r8.u32);
	// lwz r8,20(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 20);
	// stw r8,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r8.u32);
	// add r9,r10,r30
	ctx.r9.u64 = ctx.r10.u64 + ctx.r30.u64;
	// lwz r8,24(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 24);
	// stw r8,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r8.u32);
	// lwz r8,28(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 28);
	// stw r8,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r8.u32);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lwz r8,32(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 32);
	// stw r8,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r8.u32);
	// lwz r8,36(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 36);
	// stw r8,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r8.u32);
	// add r9,r10,r30
	ctx.r9.u64 = ctx.r10.u64 + ctx.r30.u64;
	// lwz r8,40(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 40);
	// stw r8,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r8.u32);
	// lwz r8,44(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 44);
	// stw r8,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r8.u32);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lwz r8,48(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 48);
	// stw r8,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r8.u32);
	// lwz r8,52(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + 52);
	// stw r8,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r8.u32);
	// lwz r9,56(r11)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r11.u32 + 56);
	// stw r9,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r9.u32);
	// lwz r9,60(r11)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r11.u32 + 60);
	// stw r9,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r9.u32);
	// b 0x82959098
	goto loc_82959098;
loc_82958E84:
	// lbz r8,0(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// stb r8,0(r26)
	REX_STORE_U8(ctx.r26.u32 + 0, ctx.r8.u8);
	// lbz r8,1(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 1);
	// stb r8,1(r26)
	REX_STORE_U8(ctx.r26.u32 + 1, ctx.r8.u8);
	// lbz r8,2(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 2);
	// stb r8,2(r26)
	REX_STORE_U8(ctx.r26.u32 + 2, ctx.r8.u8);
	// lbz r8,3(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 3);
	// stb r8,3(r26)
	REX_STORE_U8(ctx.r26.u32 + 3, ctx.r8.u8);
	// lbz r8,4(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 4);
	// stb r8,4(r26)
	REX_STORE_U8(ctx.r26.u32 + 4, ctx.r8.u8);
	// lbz r8,5(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 5);
	// stb r8,5(r26)
	REX_STORE_U8(ctx.r26.u32 + 5, ctx.r8.u8);
	// lbz r8,6(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 6);
	// stb r8,6(r26)
	REX_STORE_U8(ctx.r26.u32 + 6, ctx.r8.u8);
	// lbz r8,7(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 7);
	// stb r8,7(r26)
	REX_STORE_U8(ctx.r26.u32 + 7, ctx.r8.u8);
	// lbz r8,8(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 8);
	// stb r8,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r8.u8);
	// lbz r8,9(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 9);
	// stb r8,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r8.u8);
	// lbz r8,10(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 10);
	// stb r8,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r8.u8);
	// lbz r8,11(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 11);
	// stb r8,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r8.u8);
	// lbz r8,12(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 12);
	// stb r8,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r8.u8);
	// lbz r8,13(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 13);
	// stb r8,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r8.u8);
	// lbz r8,14(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 14);
	// stb r8,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r8.u8);
	// lbz r8,15(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 15);
	// stb r8,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r8.u8);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lbz r8,16(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 16);
	// stb r8,0(r9)
	REX_STORE_U8(ctx.r9.u32 + 0, ctx.r8.u8);
	// lbz r8,17(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 17);
	// stb r8,1(r9)
	REX_STORE_U8(ctx.r9.u32 + 1, ctx.r8.u8);
	// lbz r8,18(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 18);
	// stb r8,2(r9)
	REX_STORE_U8(ctx.r9.u32 + 2, ctx.r8.u8);
	// lbz r8,19(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 19);
	// stb r8,3(r9)
	REX_STORE_U8(ctx.r9.u32 + 3, ctx.r8.u8);
	// lbz r8,20(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 20);
	// stb r8,4(r9)
	REX_STORE_U8(ctx.r9.u32 + 4, ctx.r8.u8);
	// lbz r8,21(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 21);
	// stb r8,5(r9)
	REX_STORE_U8(ctx.r9.u32 + 5, ctx.r8.u8);
	// lbz r8,22(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 22);
	// stb r8,6(r9)
	REX_STORE_U8(ctx.r9.u32 + 6, ctx.r8.u8);
	// lbz r8,23(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 23);
	// stb r8,7(r9)
	REX_STORE_U8(ctx.r9.u32 + 7, ctx.r8.u8);
	// add r9,r10,r30
	ctx.r9.u64 = ctx.r10.u64 + ctx.r30.u64;
	// lbz r8,24(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 24);
	// stb r8,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r8.u8);
	// lbz r8,25(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 25);
	// stb r8,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r8.u8);
	// lbz r8,26(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 26);
	// stb r8,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r8.u8);
	// lbz r8,27(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 27);
	// stb r8,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r8.u8);
	// lbz r8,28(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 28);
	// stb r8,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r8.u8);
	// lbz r8,29(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 29);
	// stb r8,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r8.u8);
	// lbz r8,30(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 30);
	// stb r8,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r8.u8);
	// lbz r8,31(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 31);
	// stb r8,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r8.u8);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lbz r8,32(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 32);
	// stb r8,0(r9)
	REX_STORE_U8(ctx.r9.u32 + 0, ctx.r8.u8);
	// lbz r8,33(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 33);
	// stb r8,1(r9)
	REX_STORE_U8(ctx.r9.u32 + 1, ctx.r8.u8);
	// lbz r8,34(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 34);
	// stb r8,2(r9)
	REX_STORE_U8(ctx.r9.u32 + 2, ctx.r8.u8);
	// lbz r8,35(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 35);
	// stb r8,3(r9)
	REX_STORE_U8(ctx.r9.u32 + 3, ctx.r8.u8);
	// lbz r8,36(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 36);
	// stb r8,4(r9)
	REX_STORE_U8(ctx.r9.u32 + 4, ctx.r8.u8);
	// lbz r8,37(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 37);
	// stb r8,5(r9)
	REX_STORE_U8(ctx.r9.u32 + 5, ctx.r8.u8);
	// lbz r8,38(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 38);
	// stb r8,6(r9)
	REX_STORE_U8(ctx.r9.u32 + 6, ctx.r8.u8);
	// lbz r8,39(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 39);
	// stb r8,7(r9)
	REX_STORE_U8(ctx.r9.u32 + 7, ctx.r8.u8);
	// add r9,r10,r30
	ctx.r9.u64 = ctx.r10.u64 + ctx.r30.u64;
	// lbz r8,40(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 40);
	// stb r8,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r8.u8);
	// lbz r8,41(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 41);
	// stb r8,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r8.u8);
	// lbz r8,42(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 42);
	// stb r8,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r8.u8);
	// lbz r8,43(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 43);
	// stb r8,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r8.u8);
	// lbz r8,44(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 44);
	// stb r8,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r8.u8);
	// lbz r8,45(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 45);
	// stb r8,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r8.u8);
	// lbz r8,46(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 46);
	// stb r8,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r8.u8);
	// lbz r8,47(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 47);
	// stb r8,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r8.u8);
	// add r10,r9,r30
	ctx.r10.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lbz r8,48(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 48);
	// stb r8,0(r9)
	REX_STORE_U8(ctx.r9.u32 + 0, ctx.r8.u8);
	// lbz r8,49(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 49);
	// stb r8,1(r9)
	REX_STORE_U8(ctx.r9.u32 + 1, ctx.r8.u8);
	// lbz r8,50(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 50);
	// stb r8,2(r9)
	REX_STORE_U8(ctx.r9.u32 + 2, ctx.r8.u8);
	// lbz r8,51(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 51);
	// stb r8,3(r9)
	REX_STORE_U8(ctx.r9.u32 + 3, ctx.r8.u8);
	// lbz r8,52(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 52);
	// stb r8,4(r9)
	REX_STORE_U8(ctx.r9.u32 + 4, ctx.r8.u8);
	// lbz r8,53(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 53);
	// stb r8,5(r9)
	REX_STORE_U8(ctx.r9.u32 + 5, ctx.r8.u8);
	// lbz r8,54(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 54);
	// stb r8,6(r9)
	REX_STORE_U8(ctx.r9.u32 + 6, ctx.r8.u8);
	// lbz r8,55(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 55);
	// stb r8,7(r9)
	REX_STORE_U8(ctx.r9.u32 + 7, ctx.r8.u8);
	// lbz r9,56(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 56);
	// stb r9,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r9.u8);
	// lbz r9,57(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 57);
	// stb r9,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r9.u8);
	// lbz r9,58(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 58);
	// stb r9,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r9.u8);
	// lbz r9,59(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 59);
	// stb r9,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r9.u8);
	// lbz r9,60(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 60);
	// stb r9,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r9.u8);
	// lbz r9,61(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 61);
	// stb r9,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r9.u8);
	// lbz r9,62(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 62);
	// stb r9,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r9.u8);
	// lbz r9,63(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 63);
	// stb r9,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r9.u8);
loc_82959098:
	// addi r11,r11,64
	ctx.r11.s64 = ctx.r11.s64 + 64;
loc_8295909C:
	// stw r11,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r11.u32);
loc_829590A0:
	// addi r17,r17,-8
	ctx.r17.s64 = ctx.r17.s64 + -8;
	// lwz r14,104(r1)
	ctx.r14.u64 = REX_LOAD_U32(ctx.r1.u32 + 104);
	// add r24,r24,r15
	ctx.r24.u64 = ctx.r24.u64 + ctx.r15.u64;
	// addi r20,r20,8
	ctx.r20.s64 = ctx.r20.s64 + 8;
	// addi r26,r26,8
	ctx.r26.s64 = ctx.r26.s64 + 8;
	// cmplwi cr6,r17,0
	ctx.cr6.compare<uint32_t>(ctx.r17.u32, 0, ctx.xer);
	// stw r17,116(r1)
	REX_STORE_U32(ctx.r1.u32 + 116, ctx.r17.u32);
	// stw r24,108(r1)
	REX_STORE_U32(ctx.r1.u32 + 108, ctx.r24.u32);
	// stw r20,96(r1)
	REX_STORE_U32(ctx.r1.u32 + 96, ctx.r20.u32);
	// bne cr6,0x82957604
	if (!ctx.cr6.eq) goto loc_82957604;
	// stw r27,432(r1)
	REX_STORE_U32(ctx.r1.u32 + 432, ctx.r27.u32);
	// lwz r27,176(r1)
	ctx.r27.u64 = REX_LOAD_U32(ctx.r1.u32 + 176);
	// lwz r17,1348(r1)
	ctx.r17.u64 = REX_LOAD_U32(ctx.r1.u32 + 1348);
	// lwz r25,1356(r1)
	ctx.r25.u64 = REX_LOAD_U32(ctx.r1.u32 + 1356);
	// lwz r29,1396(r1)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r1.u32 + 1396);
loc_829590DC:
	// cmplwi cr6,r15,1
	ctx.cr6.compare<uint32_t>(ctx.r15.u32, 1, ctx.xer);
	// bne cr6,0x829590f0
	if (!ctx.cr6.eq) goto loc_829590F0;
	// rlwinm r11,r14,0,28,28
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r14.u32 | (ctx.r14.u64 << 32), 0) & 0x8;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x829590f8
	if (ctx.cr6.eq) goto loc_829590F8;
loc_829590F0:
	// add r23,r23,r29
	ctx.r23.u64 = ctx.r23.u64 + ctx.r29.u64;
	// stw r23,112(r1)
	REX_STORE_U32(ctx.r1.u32 + 112, ctx.r23.u32);
loc_829590F8:
	// addi r14,r14,8
	ctx.r14.s64 = ctx.r14.s64 + 8;
	// mr r24,r18
	ctx.r24.u64 = ctx.r18.u64;
	// add r20,r20,r27
	ctx.r20.u64 = ctx.r20.u64 + ctx.r27.u64;
	// add r26,r26,r27
	ctx.r26.u64 = ctx.r26.u64 + ctx.r27.u64;
	// cmplw cr6,r14,r25
	ctx.cr6.compare<uint32_t>(ctx.r14.u32, ctx.r25.u32, ctx.xer);
	// stw r14,104(r1)
	REX_STORE_U32(ctx.r1.u32 + 104, ctx.r14.u32);
	// stw r24,108(r1)
	REX_STORE_U32(ctx.r1.u32 + 108, ctx.r24.u32);
	// stw r20,96(r1)
	REX_STORE_U32(ctx.r1.u32 + 96, ctx.r20.u32);
	// blt cr6,0x8295755c
	if (ctx.cr6.lt) goto loc_8295755C;
	// lwz r11,1428(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 1428);
	// lwz r10,100(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 100);
	// stw r10,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
	// addi r1,r1,1312
	ctx.r1.s64 = ctx.r1.s64 + 1312;
	// b 0x824131c0
	__restgprlr_14(ctx, base);
	return;
loc_82959130:
	// lwz r11,1428(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 1428);
	// lwz r10,100(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 100);
	// lwz r3,84(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 84);
	// stw r10,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r10.u32);
	// addi r1,r1,1312
	ctx.r1.s64 = ctx.r1.s64 + 1312;
	// b 0x824131c0
	__restgprlr_14(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82959148) {
	REX_FUNC_PROLOGUE(sub_82959148);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413188
	ctx.lr = 0x82959150;
	__savegprlr_20(ctx, base);
	// stwu r1,-240(r1)
	ea = -240 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// mr r28,r9
	ctx.r28.u64 = ctx.r9.u64;
	// mr r26,r4
	ctx.r26.u64 = ctx.r4.u64;
	// mr r25,r5
	ctx.r25.u64 = ctx.r5.u64;
	// mr r27,r6
	ctx.r27.u64 = ctx.r6.u64;
	// lwz r30,20(r31)
	ctx.r30.u64 = REX_LOAD_U32(ctx.r31.u32 + 20);
	// mr r24,r7
	ctx.r24.u64 = ctx.r7.u64;
	// mr r23,r8
	ctx.r23.u64 = ctx.r8.u64;
	// xori r11,r30,1
	ctx.r11.u64 = ctx.r30.u64 ^ 1;
	// rlwinm r9,r11,1,0,30
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// add r9,r11,r9
	ctx.r9.u64 = ctx.r11.u64 + ctx.r9.u64;
	// rlwinm r9,r9,4,0,27
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 4) & 0xFFFFFFF0;
	// add r9,r9,r31
	ctx.r9.u64 = ctx.r9.u64 + ctx.r31.u64;
	// lwz r9,28(r9)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r9.u32 + 28);
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// beq cr6,0x82959198
	if (ctx.cr6.eq) goto loc_82959198;
	// stw r11,20(r31)
	REX_STORE_U32(ctx.r31.u32 + 20, ctx.r11.u32);
loc_82959198:
	// rlwinm r11,r10,0,11,11
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 0) & 0x100000;
	// lwz r29,20(r31)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r31.u32 + 20);
	// li r22,1
	ctx.r22.s64 = 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x8295921c
	if (ctx.cr6.eq) goto loc_8295921C;
	// rlwinm r11,r28,0,11,11
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 0) & 0x100000;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959214
	if (ctx.cr6.eq) goto loc_82959214;
	// addi r11,r1,128
	ctx.r11.s64 = ctx.r1.s64 + 128;
	// lwz r6,8(r31)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r31.u32 + 8);
	// rlwinm r7,r29,1,0,30
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 1) & 0xFFFFFFFE;
	// lwz r5,4(r31)
	ctx.r5.u64 = REX_LOAD_U32(ctx.r31.u32 + 4);
	// mr r10,r26
	ctx.r10.u64 = ctx.r26.u64;
	// stw r28,108(r1)
	REX_STORE_U32(ctx.r1.u32 + 108, ctx.r28.u32);
	// add r7,r29,r7
	ctx.r7.u64 = ctx.r29.u64 + ctx.r7.u64;
	// stw r23,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r23.u32);
	// mr r9,r24
	ctx.r9.u64 = ctx.r24.u64;
	// stw r25,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r25.u32);
	// stw r11,116(r1)
	REX_STORE_U32(ctx.r1.u32 + 116, ctx.r11.u32);
	// rlwinm r11,r30,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 1) & 0xFFFFFFFE;
	// rlwinm r7,r7,4,0,27
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 4) & 0xFFFFFFF0;
	// stw r22,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r22.u32);
	// add r11,r30,r11
	ctx.r11.u64 = ctx.r30.u64 + ctx.r11.u64;
	// add r7,r7,r31
	ctx.r7.u64 = ctx.r7.u64 + ctx.r31.u64;
	// rlwinm r11,r11,4,0,27
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 4) & 0xFFFFFFF0;
	// addi r8,r27,4
	ctx.r8.s64 = ctx.r27.s64 + 4;
	// add r11,r11,r31
	ctx.r11.u64 = ctx.r11.u64 + ctx.r31.u64;
	// lwz r3,64(r7)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r7.u32 + 64);
	// lwz r7,68(r11)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r11.u32 + 68);
	// lwz r4,64(r11)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r11.u32 + 64);
	// bl 0x82957260
	ctx.lr = 0x82959214;
	sub_82957260(ctx, base);
loc_82959214:
	// lwz r11,0(r27)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r27.u32 + 0);
	// add r27,r11,r27
	ctx.r27.u64 = ctx.r11.u64 + ctx.r27.u64;
loc_8295921C:
	// li r20,0
	ctx.r20.s64 = 0;
	// rlwinm r21,r28,0,16,16
	ctx.r21.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 0) & 0x8000;
	// cmplwi cr6,r21,0
	ctx.cr6.compare<uint32_t>(ctx.r21.u32, 0, ctx.xer);
	// stw r20,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r20.u32);
	// bne cr6,0x82959234
	if (!ctx.cr6.eq) goto loc_82959234;
	// addi r27,r27,4
	ctx.r27.s64 = ctx.r27.s64 + 4;
loc_82959234:
	// addi r11,r1,128
	ctx.r11.s64 = ctx.r1.s64 + 128;
	// lwz r6,8(r31)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r31.u32 + 8);
	// rlwinm r7,r30,1,0,30
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 1) & 0xFFFFFFFE;
	// lwz r5,4(r31)
	ctx.r5.u64 = REX_LOAD_U32(ctx.r31.u32 + 4);
	// mr r10,r26
	ctx.r10.u64 = ctx.r26.u64;
	// stw r28,108(r1)
	REX_STORE_U32(ctx.r1.u32 + 108, ctx.r28.u32);
	// add r7,r30,r7
	ctx.r7.u64 = ctx.r30.u64 + ctx.r7.u64;
	// stw r23,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r23.u32);
	// mr r9,r24
	ctx.r9.u64 = ctx.r24.u64;
	// stw r25,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r25.u32);
	// stw r11,116(r1)
	REX_STORE_U32(ctx.r1.u32 + 116, ctx.r11.u32);
	// rlwinm r11,r29,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 1) & 0xFFFFFFFE;
	// rlwinm r7,r7,4,0,27
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 4) & 0xFFFFFFF0;
	// stw r22,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r22.u32);
	// add r11,r29,r11
	ctx.r11.u64 = ctx.r29.u64 + ctx.r11.u64;
	// add r30,r7,r31
	ctx.r30.u64 = ctx.r7.u64 + ctx.r31.u64;
	// rlwinm r11,r11,4,0,27
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 4) & 0xFFFFFFF0;
	// mr r8,r27
	ctx.r8.u64 = ctx.r27.u64;
	// add r29,r11,r31
	ctx.r29.u64 = ctx.r11.u64 + ctx.r31.u64;
	// lwz r7,32(r30)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r30.u32 + 32);
	// lwz r4,28(r30)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r30.u32 + 28);
	// lwz r3,28(r29)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r29.u32 + 28);
	// bl 0x82957260
	ctx.lr = 0x82959290;
	sub_82957260(ctx, base);
	// mr r8,r3
	ctx.r8.u64 = ctx.r3.u64;
	// cmplwi cr6,r21,0
	ctx.cr6.compare<uint32_t>(ctx.r21.u32, 0, ctx.xer);
	// bne cr6,0x829592a8
	if (!ctx.cr6.eq) goto loc_829592A8;
	// lwz r11,-4(r27)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r27.u32 + -4);
	// add r11,r11,r27
	ctx.r11.u64 = ctx.r11.u64 + ctx.r27.u64;
	// addi r8,r11,-4
	ctx.r8.s64 = ctx.r11.s64 + -4;
loc_829592A8:
	// lwz r10,128(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 128);
	// rlwinm r11,r28,0,14,14
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 0) & 0x20000;
	// rlwinm r3,r10,30,2,31
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 30) & 0x3FFFFFFF;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295933c
	if (!ctx.cr6.eq) goto loc_8295933C;
	// addi r11,r1,128
	ctx.r11.s64 = ctx.r1.s64 + 128;
	// lwz r7,44(r30)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r30.u32 + 44);
	// li r27,2
	ctx.r27.s64 = 2;
	// lwz r4,40(r30)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r30.u32 + 40);
	// mr r10,r26
	ctx.r10.u64 = ctx.r26.u64;
	// lwz r3,40(r29)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r29.u32 + 40);
	// mr r9,r24
	ctx.r9.u64 = ctx.r24.u64;
	// lwz r6,16(r31)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r31.u32 + 16);
	// lwz r5,12(r31)
	ctx.r5.u64 = REX_LOAD_U32(ctx.r31.u32 + 12);
	// stw r28,108(r1)
	REX_STORE_U32(ctx.r1.u32 + 108, ctx.r28.u32);
	// stw r23,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r23.u32);
	// stw r25,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r25.u32);
	// stw r11,116(r1)
	REX_STORE_U32(ctx.r1.u32 + 116, ctx.r11.u32);
	// stw r27,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r27.u32);
	// bl 0x82957260
	ctx.lr = 0x829592F8;
	sub_82957260(ctx, base);
	// addi r11,r1,128
	ctx.r11.s64 = ctx.r1.s64 + 128;
	// lwz r7,56(r30)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r30.u32 + 56);
	// mr r8,r3
	ctx.r8.u64 = ctx.r3.u64;
	// lwz r6,16(r31)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r31.u32 + 16);
	// mr r10,r26
	ctx.r10.u64 = ctx.r26.u64;
	// lwz r5,12(r31)
	ctx.r5.u64 = REX_LOAD_U32(ctx.r31.u32 + 12);
	// mr r9,r24
	ctx.r9.u64 = ctx.r24.u64;
	// lwz r4,52(r30)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r30.u32 + 52);
	// lwz r3,52(r29)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r29.u32 + 52);
	// stw r20,128(r1)
	REX_STORE_U32(ctx.r1.u32 + 128, ctx.r20.u32);
	// stw r28,108(r1)
	REX_STORE_U32(ctx.r1.u32 + 108, ctx.r28.u32);
	// stw r11,116(r1)
	REX_STORE_U32(ctx.r1.u32 + 116, ctx.r11.u32);
	// stw r23,100(r1)
	REX_STORE_U32(ctx.r1.u32 + 100, ctx.r23.u32);
	// stw r27,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r27.u32);
	// stw r25,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r25.u32);
	// bl 0x82957260
	ctx.lr = 0x82959338;
	sub_82957260(ctx, base);
	// lwz r3,128(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 128);
loc_8295933C:
	// addi r1,r1,240
	ctx.r1.s64 = ctx.r1.s64 + 240;
	// b 0x824131d8
	__restgprlr_20(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82959348) {
	REX_FUNC_PROLOGUE(sub_82959348);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a8
	ctx.lr = 0x82959350;
	__savegprlr_28(ctx, base);
	// stwu r1,-144(r1)
	ea = -144 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// mr r30,r4
	ctx.r30.u64 = ctx.r4.u64;
	// mr r28,r5
	ctx.r28.u64 = ctx.r5.u64;
	// mr r29,r6
	ctx.r29.u64 = ctx.r6.u64;
	// lwz r11,244(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 244);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959378
	if (ctx.cr6.eq) goto loc_82959378;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x82959378;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
loc_82959378:
	// cmpwi cr6,r30,-1
	ctx.cr6.compare<int32_t>(ctx.r30.s32, -1, ctx.xer);
	// beq cr6,0x829593a8
	if (ctx.cr6.eq) goto loc_829593A8;
	// lwz r11,88(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 88);
	// cmplw cr6,r11,r30
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r30.u32, ctx.xer);
	// beq cr6,0x829593a8
	if (ctx.cr6.eq) goto loc_829593A8;
	// lwz r11,124(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 124);
	// li r6,0
	ctx.r6.s64 = 0;
	// li r5,0
	ctx.r5.s64 = 0;
	// lwz r3,84(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 84);
	// add r4,r11,r30
	ctx.r4.u64 = ctx.r11.u64 + ctx.r30.u64;
	// bl 0x8240df58
	ctx.lr = 0x829593A4;
	sub_8240DF58(ctx, base);
	// stw r30,88(r31)
	REX_STORE_U32(ctx.r31.u32 + 88, ctx.r30.u32);
loc_829593A8:
	// li r7,0
	ctx.r7.s64 = 0;
	// lwz r3,84(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 84);
	// addi r6,r1,80
	ctx.r6.s64 = ctx.r1.s64 + 80;
	// mr r5,r29
	ctx.r5.u64 = ctx.r29.u64;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// bl 0x8240e3d8
	ctx.lr = 0x829593C0;
	sub_8240E3D8(ctx, base);
	// lwz r3,80(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// cmplw cr6,r3,r29
	ctx.cr6.compare<uint32_t>(ctx.r3.u32, ctx.r29.u32, ctx.xer);
	// beq cr6,0x829593d4
	if (ctx.cr6.eq) goto loc_829593D4;
	// li r11,1
	ctx.r11.s64 = 1;
	// stw r11,32(r31)
	REX_STORE_U32(ctx.r31.u32 + 32, ctx.r11.u32);
loc_829593D4:
	// lwz r11,88(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 88);
	// add r11,r11,r3
	ctx.r11.u64 = ctx.r11.u64 + ctx.r3.u64;
	// stw r11,88(r31)
	REX_STORE_U32(ctx.r31.u32 + 88, ctx.r11.u32);
	// lwz r11,88(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 88);
	// stw r11,92(r31)
	REX_STORE_U32(ctx.r31.u32 + 92, ctx.r11.u32);
	// lwz r11,128(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 128);
	// lwz r10,92(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 92);
	// lwz r9,64(r31)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r31.u32 + 64);
	// subf r11,r10,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r10.u64;
	// cmplw cr6,r11,r9
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r9.u32, ctx.xer);
	// blt cr6,0x82959404
	if (ctx.cr6.lt) goto loc_82959404;
	// lwz r11,64(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 64);
loc_82959404:
	// lwz r10,252(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 252);
	// stw r11,72(r31)
	REX_STORE_U32(ctx.r31.u32 + 72, ctx.r11.u32);
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x82959424
	if (ctx.cr6.eq) goto loc_82959424;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mtctr r10
	ctx.ctr.u64 = ctx.r10.u64;
	// bctrl 
	ctx.lr = 0x82959420;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
	// lwz r3,80(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
loc_82959424:
	// addi r10,r3,3
	ctx.r10.s64 = ctx.r3.s64 + 3;
	// mr r11,r28
	ctx.r11.u64 = ctx.r28.u64;
	// rlwinm r10,r10,30,2,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 30) & 0x3FFFFFFF;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// beq cr6,0x8295945c
	if (ctx.cr6.eq) goto loc_8295945C;
loc_8295943C:
	// lwbrx r9,0,r11
	ctx.r9.u64 = __builtin_bswap32(REX_LOAD_U32(ctx.r11.u32));
	// addi r10,r10,-1
	ctx.r10.s64 = ctx.r10.s64 + -1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// stw r9,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r9.u32);
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// bne cr6,0x8295943c
	if (!ctx.cr6.eq) goto loc_8295943C;
loc_8295945C:
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131f8
	__restgprlr_28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82959468) {
	REX_FUNC_PROLOGUE(sub_82959468);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a8
	ctx.lr = 0x82959470;
	__savegprlr_28(ctx, base);
	// stwu r1,-128(r1)
	ea = -128 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// clrldi r11,r4,32
	ctx.r11.u64 = ctx.r4.u64 & 0xFFFFFFFF;
	// mr r29,r5
	ctx.r29.u64 = ctx.r5.u64;
	// mulli r11,r11,1000
	ctx.r11.s64 = static_cast<int64_t>(ctx.r11.u64 * static_cast<uint64_t>(1000));
	// lwz r10,132(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 132);
	// divdu r11,r11,r10
	ctx.r11.u64 = ctx.r10.u64 ? ctx.r11.u64 / ctx.r10.u64 : 0;
	// rotlwi r28,r11,0
	ctx.r28.u64 = __builtin_rotateleft32(ctx.r11.u32, 0);
	// bl 0x82954790
	ctx.lr = 0x82959494;
	sub_82954790(ctx, base);
	// mr r30,r3
	ctx.r30.u64 = ctx.r3.u64;
	// lwz r10,136(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 136);
	// subf r11,r30,r28
	ctx.r11.u64 = ctx.r28.u64 - ctx.r30.u64;
	// add r11,r11,r10
	ctx.r11.u64 = ctx.r11.u64 + ctx.r10.u64;
	// add r11,r11,r29
	ctx.r11.u64 = ctx.r11.u64 + ctx.r29.u64;
	// stw r11,136(r31)
	REX_STORE_U32(ctx.r31.u32 + 136, ctx.r11.u32);
	// lwz r11,136(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 136);
	// cmpwi cr6,r11,0
	ctx.cr6.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
	// ble cr6,0x829594ec
	if (!ctx.cr6.gt) goto loc_829594EC;
loc_829594B8:
	// bl 0x82954790
	ctx.lr = 0x829594BC;
	sub_82954790(ctx, base);
	// lwz r11,136(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 136);
	// subf r10,r30,r3
	ctx.r10.u64 = ctx.r3.u64 - ctx.r30.u64;
	// cmpw cr6,r10,r11
	ctx.cr6.compare<int32_t>(ctx.r10.s32, ctx.r11.s32, ctx.xer);
	// blt cr6,0x829594b8
	if (ctx.cr6.lt) goto loc_829594B8;
	// lwz r10,136(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 136);
	// subf r11,r3,r30
	ctx.r11.u64 = ctx.r30.u64 - ctx.r3.u64;
	// mr r30,r3
	ctx.r30.u64 = ctx.r3.u64;
	// add r11,r11,r10
	ctx.r11.u64 = ctx.r11.u64 + ctx.r10.u64;
	// stw r11,136(r31)
	REX_STORE_U32(ctx.r31.u32 + 136, ctx.r11.u32);
	// lwz r11,136(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 136);
	// cmpwi cr6,r11,0
	ctx.cr6.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
	// bgt cr6,0x829594b8
	if (ctx.cr6.gt) goto loc_829594B8;
loc_829594EC:
	// addi r1,r1,128
	ctx.r1.s64 = ctx.r1.s64 + 128;
	// b 0x824131f8
	__restgprlr_28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_829594F8) {
	REX_FUNC_PROLOGUE(sub_829594F8);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413190
	ctx.lr = 0x82959500;
	__savegprlr_22(ctx, base);
	// stwu r1,-192(r1)
	ea = -192 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// li r25,0
	ctx.r25.s64 = 0;
	// mr r28,r6
	ctx.r28.u64 = ctx.r6.u64;
	// mr r30,r5
	ctx.r30.u64 = ctx.r5.u64;
	// mr r27,r7
	ctx.r27.u64 = ctx.r7.u64;
	// lwz r11,32(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 32);
	// mr r23,r25
	ctx.r23.u64 = ctx.r25.u64;
	// stw r25,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r25.u32);
	// mr r22,r28
	ctx.r22.u64 = ctx.r28.u64;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x8295953c
	if (ctx.cr6.eq) goto loc_8295953C;
	// li r3,0
	ctx.r3.s64 = 0;
	// addi r1,r1,192
	ctx.r1.s64 = ctx.r1.s64 + 192;
	// b 0x824131e0
	__restgprlr_22(ctx, base);
	return;
loc_8295953C:
	// bl 0x82954790
	ctx.lr = 0x82959540;
	sub_82954790(ctx, base);
	// mr r24,r3
	ctx.r24.u64 = ctx.r3.u64;
	// cmpwi cr6,r30,-1
	ctx.cr6.compare<int32_t>(ctx.r30.s32, -1, ctx.xer);
	// beq cr6,0x82959620
	if (ctx.cr6.eq) goto loc_82959620;
	// lwz r11,92(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 92);
	// cmplw cr6,r11,r30
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r30.u32, ctx.xer);
	// beq cr6,0x82959620
	if (ctx.cr6.eq) goto loc_82959620;
	// lwz r11,244(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 244);
	// li r23,1
	ctx.r23.s64 = 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959574
	if (ctx.cr6.eq) goto loc_82959574;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x82959574;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
loc_82959574:
	// lwz r11,92(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 92);
	// cmplw cr6,r30,r11
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, ctx.r11.u32, ctx.xer);
	// ble cr6,0x829595e4
	if (!ctx.cr6.gt) goto loc_829595E4;
	// lwz r11,88(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 88);
	// cmplw cr6,r30,r11
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, ctx.r11.u32, ctx.xer);
	// bgt cr6,0x829595e4
	if (ctx.cr6.gt) goto loc_829595E4;
	// lwz r11,92(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 92);
	// stw r30,92(r31)
	REX_STORE_U32(ctx.r31.u32 + 92, ctx.r30.u32);
	// lwz r10,100(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 100);
	// subf r11,r11,r30
	ctx.r11.u64 = ctx.r30.u64 - ctx.r11.u64;
	// add r10,r10,r11
	ctx.r10.u64 = ctx.r10.u64 + ctx.r11.u64;
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// stw r10,100(r31)
	REX_STORE_U32(ctx.r31.u32 + 100, ctx.r10.u32);
	// lwz r10,76(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 76);
	// subf r10,r11,r10
	ctx.r10.u64 = ctx.r10.u64 - ctx.r11.u64;
	// stw r10,76(r31)
	REX_STORE_U32(ctx.r31.u32 + 76, ctx.r10.u32);
	// lwz r10,96(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 96);
	// add r11,r10,r11
	ctx.r11.u64 = ctx.r10.u64 + ctx.r11.u64;
	// stw r11,96(r31)
	REX_STORE_U32(ctx.r31.u32 + 96, ctx.r11.u32);
	// lwz r11,112(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 112);
	// lwz r10,96(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 96);
	// cmplw cr6,r10,r11
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r11.u32, ctx.xer);
	// ble cr6,0x82959620
	if (!ctx.cr6.gt) goto loc_82959620;
	// lwz r11,64(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 64);
	// lwz r10,96(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 96);
	// subf r11,r11,r10
	ctx.r11.u64 = ctx.r10.u64 - ctx.r11.u64;
	// stw r11,96(r31)
	REX_STORE_U32(ctx.r31.u32 + 96, ctx.r11.u32);
	// b 0x82959620
	goto loc_82959620;
loc_829595E4:
	// lwz r11,124(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 124);
	// li r6,0
	ctx.r6.s64 = 0;
	// li r5,0
	ctx.r5.s64 = 0;
	// lwz r3,84(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 84);
	// add r4,r11,r30
	ctx.r4.u64 = ctx.r11.u64 + ctx.r30.u64;
	// bl 0x8240df58
	ctx.lr = 0x829595FC;
	sub_8240DF58(ctx, base);
	// stw r30,88(r31)
	REX_STORE_U32(ctx.r31.u32 + 88, ctx.r30.u32);
	// stw r30,92(r31)
	REX_STORE_U32(ctx.r31.u32 + 92, ctx.r30.u32);
	// lwz r11,64(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 64);
	// stw r11,100(r31)
	REX_STORE_U32(ctx.r31.u32 + 100, ctx.r11.u32);
	// stw r25,76(r31)
	REX_STORE_U32(ctx.r31.u32 + 76, ctx.r25.u32);
	// lwz r11,108(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 108);
	// stw r11,96(r31)
	REX_STORE_U32(ctx.r31.u32 + 96, ctx.r11.u32);
	// lwz r11,108(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 108);
	// stw r11,116(r31)
	REX_STORE_U32(ctx.r31.u32 + 116, ctx.r11.u32);
loc_82959620:
	// addi r26,r31,76
	ctx.r26.s64 = ctx.r31.s64 + 76;
loc_82959624:
	// lwz r30,0(r26)
	ctx.r30.u64 = REX_LOAD_U32(ctx.r26.u32 + 0);
	// cmplwi cr6,r30,0
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, 0, ctx.xer);
	// beq cr6,0x8295972c
	if (ctx.cr6.eq) goto loc_8295972C;
	// cmplw cr6,r30,r27
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, ctx.r27.u32, ctx.xer);
	// ble cr6,0x8295963c
	if (!ctx.cr6.gt) goto loc_8295963C;
	// mr r30,r27
	ctx.r30.u64 = ctx.r27.u64;
loc_8295963C:
	// lwz r11,92(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 92);
	// subf r27,r30,r27
	ctx.r27.u64 = ctx.r27.u64 - ctx.r30.u64;
	// add r25,r30,r25
	ctx.r25.u64 = ctx.r30.u64 + ctx.r25.u64;
	// add r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 + ctx.r30.u64;
	// stw r11,92(r31)
	REX_STORE_U32(ctx.r31.u32 + 92, ctx.r11.u32);
	// lwz r11,112(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 112);
	// lwz r10,96(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 96);
	// subf r29,r10,r11
	ctx.r29.u64 = ctx.r11.u64 - ctx.r10.u64;
	// cmplw cr6,r29,r30
	ctx.cr6.compare<uint32_t>(ctx.r29.u32, ctx.r30.u32, ctx.xer);
	// bgt cr6,0x829596cc
	if (ctx.cr6.gt) goto loc_829596CC;
	// mr r5,r29
	ctx.r5.u64 = ctx.r29.u64;
	// lwz r4,96(r31)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r31.u32 + 96);
	// mr r3,r28
	ctx.r3.u64 = ctx.r28.u64;
	// bl 0x82413620
	ctx.lr = 0x82959674;
	sub_82413620(ctx, base);
	// lwz r11,108(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 108);
	// add r28,r29,r28
	ctx.r28.u64 = ctx.r29.u64 + ctx.r28.u64;
	// subf r30,r29,r30
	ctx.r30.u64 = ctx.r30.u64 - ctx.r29.u64;
	// neg r8,r29
	ctx.r8.s64 = static_cast<int64_t>(-ctx.r29.u64);
	// stw r11,96(r31)
	REX_STORE_U32(ctx.r31.u32 + 96, ctx.r11.u32);
loc_82959688:
	// mfmsr r7
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.r7.u64 = REX_CHECK_GLOBAL_LOCK();
	// mtmsrd r13,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r13.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_ENTER_GLOBAL_LOCK();
	// lwarx r10,0,r26
	ea = ctx.r26.u32;
	ctx.reserved.u32 = *(uint32_t*)REX_RAW_ADDR(ea);
	ctx.r10.u64 = __builtin_bswap32(ctx.reserved.u32);
	// add r9,r8,r10
	ctx.r9.u64 = ctx.r8.u64 + ctx.r10.u64;
	// stwcx. r9,0,r26
	ea = ctx.r26.u32;
	ctx.cr0.lt = 0;
	ctx.cr0.gt = 0;
	ctx.cr0.eq = __sync_bool_compare_and_swap(reinterpret_cast<uint32_t*>(REX_RAW_ADDR(ea)), ctx.reserved.s32, __builtin_bswap32(ctx.r9.s32));
	ctx.cr0.so = ctx.xer.so;
	// mtmsrd r7,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r7.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_LEAVE_GLOBAL_LOCK();
	// bne 0x82959688
	if (!ctx.cr0.eq) goto loc_82959688;
	// addi r11,r31,100
	ctx.r11.s64 = ctx.r31.s64 + 100;
loc_829596A8:
	// mfmsr r8
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.r8.u64 = REX_CHECK_GLOBAL_LOCK();
	// mtmsrd r13,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r13.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_ENTER_GLOBAL_LOCK();
	// lwarx r10,0,r11
	ea = ctx.r11.u32;
	ctx.reserved.u32 = *(uint32_t*)REX_RAW_ADDR(ea);
	ctx.r10.u64 = __builtin_bswap32(ctx.reserved.u32);
	// add r9,r29,r10
	ctx.r9.u64 = ctx.r29.u64 + ctx.r10.u64;
	// stwcx. r9,0,r11
	ea = ctx.r11.u32;
	ctx.cr0.lt = 0;
	ctx.cr0.gt = 0;
	ctx.cr0.eq = __sync_bool_compare_and_swap(reinterpret_cast<uint32_t*>(REX_RAW_ADDR(ea)), ctx.reserved.s32, __builtin_bswap32(ctx.r9.s32));
	ctx.cr0.so = ctx.xer.so;
	// mtmsrd r8,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r8.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_LEAVE_GLOBAL_LOCK();
	// bne 0x829596a8
	if (!ctx.cr0.eq) goto loc_829596A8;
	// cmplwi cr6,r30,0
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, 0, ctx.xer);
	// beq cr6,0x8295972c
	if (ctx.cr6.eq) goto loc_8295972C;
loc_829596CC:
	// mr r5,r30
	ctx.r5.u64 = ctx.r30.u64;
	// lwz r4,96(r31)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r31.u32 + 96);
	// mr r3,r28
	ctx.r3.u64 = ctx.r28.u64;
	// bl 0x82413620
	ctx.lr = 0x829596DC;
	sub_82413620(ctx, base);
	// lwz r11,96(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 96);
	// add r28,r30,r28
	ctx.r28.u64 = ctx.r30.u64 + ctx.r28.u64;
	// add r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 + ctx.r30.u64;
	// neg r8,r30
	ctx.r8.s64 = static_cast<int64_t>(-ctx.r30.u64);
	// stw r11,96(r31)
	REX_STORE_U32(ctx.r31.u32 + 96, ctx.r11.u32);
loc_829596F0:
	// mfmsr r7
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.r7.u64 = REX_CHECK_GLOBAL_LOCK();
	// mtmsrd r13,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r13.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_ENTER_GLOBAL_LOCK();
	// lwarx r10,0,r26
	ea = ctx.r26.u32;
	ctx.reserved.u32 = *(uint32_t*)REX_RAW_ADDR(ea);
	ctx.r10.u64 = __builtin_bswap32(ctx.reserved.u32);
	// add r9,r8,r10
	ctx.r9.u64 = ctx.r8.u64 + ctx.r10.u64;
	// stwcx. r9,0,r26
	ea = ctx.r26.u32;
	ctx.cr0.lt = 0;
	ctx.cr0.gt = 0;
	ctx.cr0.eq = __sync_bool_compare_and_swap(reinterpret_cast<uint32_t*>(REX_RAW_ADDR(ea)), ctx.reserved.s32, __builtin_bswap32(ctx.r9.s32));
	ctx.cr0.so = ctx.xer.so;
	// mtmsrd r7,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r7.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_LEAVE_GLOBAL_LOCK();
	// bne 0x829596f0
	if (!ctx.cr0.eq) goto loc_829596F0;
	// addi r11,r31,100
	ctx.r11.s64 = ctx.r31.s64 + 100;
loc_82959710:
	// mfmsr r8
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.r8.u64 = REX_CHECK_GLOBAL_LOCK();
	// mtmsrd r13,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r13.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_ENTER_GLOBAL_LOCK();
	// lwarx r10,0,r11
	ea = ctx.r11.u32;
	ctx.reserved.u32 = *(uint32_t*)REX_RAW_ADDR(ea);
	ctx.r10.u64 = __builtin_bswap32(ctx.reserved.u32);
	// add r9,r30,r10
	ctx.r9.u64 = ctx.r30.u64 + ctx.r10.u64;
	// stwcx. r9,0,r11
	ea = ctx.r11.u32;
	ctx.cr0.lt = 0;
	ctx.cr0.gt = 0;
	ctx.cr0.eq = __sync_bool_compare_and_swap(reinterpret_cast<uint32_t*>(REX_RAW_ADDR(ea)), ctx.reserved.s32, __builtin_bswap32(ctx.r9.s32));
	ctx.cr0.so = ctx.xer.so;
	// mtmsrd r8,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r8.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_LEAVE_GLOBAL_LOCK();
	// bne 0x82959710
	if (!ctx.cr0.eq) goto loc_82959710;
loc_8295972C:
	// cmplwi cr6,r27,0
	ctx.cr6.compare<uint32_t>(ctx.r27.u32, 0, ctx.xer);
	// beq cr6,0x829597f0
	if (ctx.cr6.eq) goto loc_829597F0;
	// cmpwi cr6,r23,0
	ctx.cr6.compare<int32_t>(ctx.r23.s32, 0, ctx.xer);
	// bne cr6,0x8295975c
	if (!ctx.cr6.eq) goto loc_8295975C;
	// lwz r11,244(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 244);
	// li r23,1
	ctx.r23.s64 = 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959624
	if (ctx.cr6.eq) goto loc_82959624;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x82959758;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
	// b 0x82959624
	goto loc_82959624;
loc_8295975C:
	// stw r25,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r25.u32);
	// bl 0x82954790
	ctx.lr = 0x82959764;
	sub_82954790(ctx, base);
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r6,r1,84
	ctx.r6.s64 = ctx.r1.s64 + 84;
	// mr r5,r27
	ctx.r5.u64 = ctx.r27.u64;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// mr r30,r3
	ctx.r30.u64 = ctx.r3.u64;
	// lwz r3,84(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 84);
	// bl 0x8240e3d8
	ctx.lr = 0x82959780;
	sub_8240E3D8(ctx, base);
	// lwz r4,84(r1)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r1.u32 + 84);
	// cmplw cr6,r4,r27
	ctx.cr6.compare<uint32_t>(ctx.r4.u32, ctx.r27.u32, ctx.xer);
	// bge cr6,0x82959794
	if (!ctx.cr6.lt) goto loc_82959794;
	// li r11,1
	ctx.r11.s64 = 1;
	// stw r11,32(r31)
	REX_STORE_U32(ctx.r31.u32 + 32, ctx.r11.u32);
loc_82959794:
	// lwz r11,88(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 88);
	// add r25,r4,r25
	ctx.r25.u64 = ctx.r4.u64 + ctx.r25.u64;
	// add r11,r11,r4
	ctx.r11.u64 = ctx.r11.u64 + ctx.r4.u64;
	// stw r25,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r25.u32);
	// stw r11,88(r31)
	REX_STORE_U32(ctx.r31.u32 + 88, ctx.r11.u32);
	// lwz r11,92(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 92);
	// add r11,r11,r4
	ctx.r11.u64 = ctx.r11.u64 + ctx.r4.u64;
	// stw r11,92(r31)
	REX_STORE_U32(ctx.r31.u32 + 92, ctx.r11.u32);
	// lwz r11,40(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 40);
	// add r11,r11,r4
	ctx.r11.u64 = ctx.r11.u64 + ctx.r4.u64;
	// stw r11,40(r31)
	REX_STORE_U32(ctx.r31.u32 + 40, ctx.r11.u32);
	// lwz r11,132(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 132);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x829597d8
	if (ctx.cr6.eq) goto loc_829597D8;
	// mr r5,r30
	ctx.r5.u64 = ctx.r30.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x82959468
	ctx.lr = 0x829597D8;
	sub_82959468(ctx, base);
loc_829597D8:
	// bl 0x82954790
	ctx.lr = 0x829597DC;
	sub_82954790(ctx, base);
	// lwz r9,48(r31)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r31.u32 + 48);
	// subf r10,r30,r3
	ctx.r10.u64 = ctx.r3.u64 - ctx.r30.u64;
	// add r10,r10,r9
	ctx.r10.u64 = ctx.r10.u64 + ctx.r9.u64;
	// stw r10,48(r31)
	REX_STORE_U32(ctx.r31.u32 + 48, ctx.r10.u32);
	// b 0x829597f8
	goto loc_829597F8;
loc_829597F0:
	// stw r25,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r25.u32);
	// bl 0x82954790
	ctx.lr = 0x829597F8;
	sub_82954790(ctx, base);
loc_829597F8:
	// lwz r10,52(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 52);
	// subf r11,r24,r3
	ctx.r11.u64 = ctx.r3.u64 - ctx.r24.u64;
	// add r11,r11,r10
	ctx.r11.u64 = ctx.r11.u64 + ctx.r10.u64;
	// stw r11,52(r31)
	REX_STORE_U32(ctx.r31.u32 + 52, ctx.r11.u32);
	// lwz r11,128(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 128);
	// lwz r10,92(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 92);
	// subf r11,r10,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r10.u64;
	// lwz r10,64(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 64);
	// cmplw cr6,r11,r10
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
	// stw r11,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r11.u32);
	// blt cr6,0x82959828
	if (ctx.cr6.lt) goto loc_82959828;
	// lwz r11,64(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 64);
loc_82959828:
	// stw r11,72(r31)
	REX_STORE_U32(ctx.r31.u32 + 72, ctx.r11.u32);
	// lwz r11,0(r26)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r26.u32 + 0);
	// lwz r10,72(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 72);
	// addis r11,r11,2
	ctx.r11.s64 = ctx.r11.s64 + 131072;
	// cmplw cr6,r11,r10
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
	// ble cr6,0x82959848
	if (!ctx.cr6.gt) goto loc_82959848;
	// lwz r11,0(r26)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r26.u32 + 0);
	// stw r11,72(r31)
	REX_STORE_U32(ctx.r31.u32 + 72, ctx.r11.u32);
loc_82959848:
	// cmpwi cr6,r23,0
	ctx.cr6.compare<int32_t>(ctx.r23.s32, 0, ctx.xer);
	// beq cr6,0x82959868
	if (ctx.cr6.eq) goto loc_82959868;
	// lwz r11,252(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 252);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959868
	if (ctx.cr6.eq) goto loc_82959868;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x82959868;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
loc_82959868:
	// addi r10,r25,3
	ctx.r10.s64 = ctx.r25.s64 + 3;
	// mr r11,r22
	ctx.r11.u64 = ctx.r22.u64;
	// rlwinm r10,r10,30,2,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 30) & 0x3FFFFFFF;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// stw r11,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r11.u32);
	// beq cr6,0x829598a0
	if (ctx.cr6.eq) goto loc_829598A0;
loc_82959880:
	// lwbrx r9,0,r11
	ctx.r9.u64 = __builtin_bswap32(REX_LOAD_U32(ctx.r11.u32));
	// addi r10,r10,-1
	ctx.r10.s64 = ctx.r10.s64 + -1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// stw r9,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r9.u32);
	// stw r11,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r11.u32);
	// bne cr6,0x82959880
	if (!ctx.cr6.eq) goto loc_82959880;
loc_829598A0:
	// mr r3,r25
	ctx.r3.u64 = ctx.r25.u64;
	// addi r1,r1,192
	ctx.r1.s64 = ctx.r1.s64 + 192;
	// b 0x824131e0
	__restgprlr_22(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_829598B0) {
	REX_FUNC_PROLOGUE(sub_829598B0);
	// addis r10,r4,2
	ctx.r10.s64 = ctx.r4.s64 + 131072;
	// lis r11,4
	ctx.r11.s64 = 262144;
	// addi r10,r10,-1
	ctx.r10.s64 = ctx.r10.s64 + -1;
	// ori r11,r11,4096
	ctx.r11.u64 = ctx.r11.u64 | 4096;
	// rlwinm r10,r10,0,0,14
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 0) & 0xFFFE0000;
	// addi r3,r10,4096
	ctx.r3.s64 = ctx.r10.s64 + 4096;
	// cmplw cr6,r3,r11
	ctx.cr6.compare<uint32_t>(ctx.r3.u32, ctx.r11.u32, ctx.xer);
	// bgelr cr6
	if (!ctx.cr6.lt) return;
	// mr r3,r11
	ctx.r3.u64 = ctx.r11.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_829598D8) {
	REX_FUNC_PROLOGUE(sub_829598D8);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a4
	ctx.lr = 0x829598E0;
	__savegprlr_27(ctx, base);
	// stwu r1,-128(r1)
	ea = -128 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// mr r30,r4
	ctx.r30.u64 = ctx.r4.u64;
	// mr r29,r5
	ctx.r29.u64 = ctx.r5.u64;
	// mr r28,r6
	ctx.r28.u64 = ctx.r6.u64;
	// mr r27,r7
	ctx.r27.u64 = ctx.r7.u64;
	// lwz r11,244(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 244);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x8295990c
	if (ctx.cr6.eq) goto loc_8295990C;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x8295990C;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
loc_8295990C:
	// addi r11,r30,4095
	ctx.r11.s64 = ctx.r30.s64 + 4095;
	// lwz r9,252(r31)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r31.u32 + 252);
	// rlwinm r10,r29,0,0,14
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 0) & 0xFFFE0000;
	// rlwinm r11,r11,0,0,19
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 0) & 0xFFFFF000;
	// li r8,0
	ctx.r8.s64 = 0;
	// add r7,r11,r10
	ctx.r7.u64 = ctx.r11.u64 + ctx.r10.u64;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// stw r11,108(r31)
	REX_STORE_U32(ctx.r31.u32 + 108, ctx.r11.u32);
	// stw r11,96(r31)
	REX_STORE_U32(ctx.r31.u32 + 96, ctx.r11.u32);
	// stw r11,116(r31)
	REX_STORE_U32(ctx.r31.u32 + 116, ctx.r11.u32);
	// stw r7,112(r31)
	REX_STORE_U32(ctx.r31.u32 + 112, ctx.r7.u32);
	// stw r10,64(r31)
	REX_STORE_U32(ctx.r31.u32 + 64, ctx.r10.u32);
	// stw r10,100(r31)
	REX_STORE_U32(ctx.r31.u32 + 100, ctx.r10.u32);
	// stw r8,76(r31)
	REX_STORE_U32(ctx.r31.u32 + 76, ctx.r8.u32);
	// stw r28,128(r31)
	REX_STORE_U32(ctx.r31.u32 + 128, ctx.r28.u32);
	// stw r27,132(r31)
	REX_STORE_U32(ctx.r31.u32 + 132, ctx.r27.u32);
	// beq cr6,0x8295995c
	if (ctx.cr6.eq) goto loc_8295995C;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mtctr r9
	ctx.ctr.u64 = ctx.r9.u64;
	// bctrl 
	ctx.lr = 0x8295995C;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
loc_8295995C:
	// addi r1,r1,128
	ctx.r1.s64 = ctx.r1.s64 + 128;
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82959968) {
	REX_FUNC_PROLOGUE(sub_82959968);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// lwz r11,244(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 244);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959990
	if (ctx.cr6.eq) goto loc_82959990;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x82959990;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
loc_82959990:
	// lwz r11,120(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 120);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x829599a4
	if (!ctx.cr6.eq) goto loc_829599A4;
	// lwz r3,84(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 84);
	// bl 0x8240c348
	ctx.lr = 0x829599A4;
	sub_8240C348(ctx, base);
loc_829599A4:
	// lwz r11,252(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 252);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x829599bc
	if (ctx.cr6.eq) goto loc_829599BC;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x829599BC;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
loc_829599BC:
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_829599D0) {
	REX_FUNC_PROLOGUE(sub_829599D0);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413198
	ctx.lr = 0x829599D8;
	__savegprlr_24(ctx, base);
	// stwu r1,-160(r1)
	ea = -160 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// li r27,0
	ctx.r27.s64 = 0;
	// lwz r24,44(r31)
	ctx.r24.u64 = REX_LOAD_U32(ctx.r31.u32 + 44);
	// lwz r11,32(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 32);
	// stw r27,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r27.u32);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959a04
	if (ctx.cr6.eq) goto loc_82959A04;
loc_829599F8:
	// li r3,0
	ctx.r3.s64 = 0;
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// b 0x824131e8
	__restgprlr_24(ctx, base);
	return;
loc_82959A04:
	// lwz r11,80(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 80);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x829599f8
	if (!ctx.cr6.eq) goto loc_829599F8;
	// lwz r11,248(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 248);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959c14
	if (ctx.cr6.eq) goto loc_82959C14;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x82959A28;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
	// cmpwi cr6,r3,0
	ctx.cr6.compare<int32_t>(ctx.r3.s32, 0, ctx.xer);
	// beq cr6,0x82959c14
	if (ctx.cr6.eq) goto loc_82959C14;
	// addi r25,r31,100
	ctx.r25.s64 = ctx.r31.s64 + 100;
	// lwz r11,128(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 128);
	// lwz r10,88(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 88);
	// lis r28,2
	ctx.r28.s64 = 131072;
	// subf r29,r10,r11
	ctx.r29.u64 = ctx.r11.u64 - ctx.r10.u64;
	// lwz r9,0(r25)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r25.u32 + 0);
	// cmplw cr6,r9,r28
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, ctx.r28.u32, ctx.xer);
	// blt cr6,0x82959be4
	if (ctx.cr6.lt) goto loc_82959BE4;
	// cmplwi cr6,r29,0
	ctx.cr6.compare<uint32_t>(ctx.r29.u32, 0, ctx.xer);
	// beq cr6,0x82959be4
	if (ctx.cr6.eq) goto loc_82959BE4;
	// mr r30,r28
	ctx.r30.u64 = ctx.r28.u64;
	// cmplw cr6,r29,r28
	ctx.cr6.compare<uint32_t>(ctx.r29.u32, ctx.r28.u32, ctx.xer);
	// bge cr6,0x82959a68
	if (!ctx.cr6.lt) goto loc_82959A68;
	// mr r30,r29
	ctx.r30.u64 = ctx.r29.u64;
loc_82959A68:
	// bl 0x82954790
	ctx.lr = 0x82959A6C;
	sub_82954790(ctx, base);
	// lwz r11,88(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 88);
	// mr r26,r3
	ctx.r26.u64 = ctx.r3.u64;
	// clrlwi r11,r11,15
	ctx.r11.u64 = ctx.r11.u32 & 0x1FFFF;
	// cmplw cr6,r11,r30
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r30.u32, ctx.xer);
	// bgt cr6,0x82959a88
	if (ctx.cr6.gt) goto loc_82959A88;
	// cmplw cr6,r30,r29
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, ctx.r29.u32, ctx.xer);
	// bne cr6,0x82959a90
	if (!ctx.cr6.eq) goto loc_82959A90;
loc_82959A88:
	// mr r11,r27
	ctx.r11.u64 = ctx.r27.u64;
	// b 0x82959aa4
	goto loc_82959AA4;
loc_82959A90:
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959aa4
	if (ctx.cr6.eq) goto loc_82959AA4;
	// lwz r10,96(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 96);
	// add r10,r10,r11
	ctx.r10.u64 = ctx.r10.u64 + ctx.r11.u64;
	// stw r10,96(r31)
	REX_STORE_U32(ctx.r31.u32 + 96, ctx.r10.u32);
loc_82959AA4:
	// li r29,1
	ctx.r29.s64 = 1;
	// subf r30,r11,r30
	ctx.r30.u64 = ctx.r30.u64 - ctx.r11.u64;
	// li r7,0
	ctx.r7.s64 = 0;
	// addi r6,r1,80
	ctx.r6.s64 = ctx.r1.s64 + 80;
	// mr r5,r30
	ctx.r5.u64 = ctx.r30.u64;
	// stw r29,36(r31)
	REX_STORE_U32(ctx.r31.u32 + 36, ctx.r29.u32);
	// lwz r10,116(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 116);
	// lwz r3,84(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 84);
	// add r4,r10,r11
	ctx.r4.u64 = ctx.r10.u64 + ctx.r11.u64;
	// bl 0x8240e3d8
	ctx.lr = 0x82959ACC;
	sub_8240E3D8(ctx, base);
	// lwz r3,80(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// stw r27,36(r31)
	REX_STORE_U32(ctx.r31.u32 + 36, ctx.r27.u32);
	// cmplw cr6,r3,r30
	ctx.cr6.compare<uint32_t>(ctx.r3.u32, ctx.r30.u32, ctx.xer);
	// beq cr6,0x82959ae0
	if (ctx.cr6.eq) goto loc_82959AE0;
	// stw r29,32(r31)
	REX_STORE_U32(ctx.r31.u32 + 32, ctx.r29.u32);
loc_82959AE0:
	// cmplwi cr6,r3,0
	ctx.cr6.compare<uint32_t>(ctx.r3.u32, 0, ctx.xer);
	// beq cr6,0x82959bf0
	if (ctx.cr6.eq) goto loc_82959BF0;
	// lwz r11,40(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 40);
	// add r11,r11,r3
	ctx.r11.u64 = ctx.r11.u64 + ctx.r3.u64;
	// stw r11,40(r31)
	REX_STORE_U32(ctx.r31.u32 + 40, ctx.r11.u32);
	// lwz r11,88(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 88);
	// add r11,r11,r3
	ctx.r11.u64 = ctx.r11.u64 + ctx.r3.u64;
	// stw r11,88(r31)
	REX_STORE_U32(ctx.r31.u32 + 88, ctx.r11.u32);
	// lwz r11,116(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 116);
	// add r11,r11,r28
	ctx.r11.u64 = ctx.r11.u64 + ctx.r28.u64;
	// stw r11,116(r31)
	REX_STORE_U32(ctx.r31.u32 + 116, ctx.r11.u32);
	// lwz r11,112(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 112);
	// lwz r10,116(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 116);
	// cmplw cr6,r10,r11
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r11.u32, ctx.xer);
	// blt cr6,0x82959b24
	if (ctx.cr6.lt) goto loc_82959B24;
	// lwz r11,108(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 108);
	// stw r11,116(r31)
	REX_STORE_U32(ctx.r31.u32 + 116, ctx.r11.u32);
loc_82959B24:
	// neg r11,r3
	ctx.r11.s64 = static_cast<int64_t>(-ctx.r3.u64);
loc_82959B28:
	// mfmsr r8
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.r8.u64 = REX_CHECK_GLOBAL_LOCK();
	// mtmsrd r13,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r13.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_ENTER_GLOBAL_LOCK();
	// lwarx r10,0,r25
	ea = ctx.r25.u32;
	ctx.reserved.u32 = *(uint32_t*)REX_RAW_ADDR(ea);
	ctx.r10.u64 = __builtin_bswap32(ctx.reserved.u32);
	// add r9,r11,r10
	ctx.r9.u64 = ctx.r11.u64 + ctx.r10.u64;
	// stwcx. r9,0,r25
	ea = ctx.r25.u32;
	ctx.cr0.lt = 0;
	ctx.cr0.gt = 0;
	ctx.cr0.eq = __sync_bool_compare_and_swap(reinterpret_cast<uint32_t*>(REX_RAW_ADDR(ea)), ctx.reserved.s32, __builtin_bswap32(ctx.r9.s32));
	ctx.cr0.so = ctx.xer.so;
	// mtmsrd r8,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r8.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_LEAVE_GLOBAL_LOCK();
	// bne 0x82959b28
	if (!ctx.cr0.eq) goto loc_82959B28;
	// lwz r7,80(r1)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// addi r11,r31,76
	ctx.r11.s64 = ctx.r31.s64 + 76;
loc_82959B4C:
	// mfmsr r8
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.r8.u64 = REX_CHECK_GLOBAL_LOCK();
	// mtmsrd r13,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r13.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_ENTER_GLOBAL_LOCK();
	// lwarx r10,0,r11
	ea = ctx.r11.u32;
	ctx.reserved.u32 = *(uint32_t*)REX_RAW_ADDR(ea);
	ctx.r10.u64 = __builtin_bswap32(ctx.reserved.u32);
	// add r9,r7,r10
	ctx.r9.u64 = ctx.r7.u64 + ctx.r10.u64;
	// stwcx. r9,0,r11
	ea = ctx.r11.u32;
	ctx.cr0.lt = 0;
	ctx.cr0.gt = 0;
	ctx.cr0.eq = __sync_bool_compare_and_swap(reinterpret_cast<uint32_t*>(REX_RAW_ADDR(ea)), ctx.reserved.s32, __builtin_bswap32(ctx.r9.s32));
	ctx.cr0.so = ctx.xer.so;
	// mtmsrd r8,1
	std::atomic_thread_fence(std::memory_order_seq_cst);
	ctx.msr = (ctx.r8.u32 & 0x8020) | (ctx.msr & ~0x8020);
	REX_LEAVE_GLOBAL_LOCK();
	// bne 0x82959b4c
	if (!ctx.cr0.eq) goto loc_82959B4C;
	// lwz r10,0(r11)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// lwz r9,68(r31)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r31.u32 + 68);
	// cmplw cr6,r10,r9
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r9.u32, ctx.xer);
	// ble cr6,0x82959b80
	if (!ctx.cr6.gt) goto loc_82959B80;
	// lwz r11,0(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// stw r11,68(r31)
	REX_STORE_U32(ctx.r31.u32 + 68, ctx.r11.u32);
loc_82959B80:
	// lwz r11,132(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 132);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959b9c
	if (ctx.cr6.eq) goto loc_82959B9C;
	// mr r5,r26
	ctx.r5.u64 = ctx.r26.u64;
	// lwz r4,80(r1)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x82959468
	ctx.lr = 0x82959B9C;
	sub_82959468(ctx, base);
loc_82959B9C:
	// bl 0x82954790
	ctx.lr = 0x82959BA0;
	sub_82954790(ctx, base);
	// lwz r10,48(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 48);
	// subf r11,r26,r3
	ctx.r11.u64 = ctx.r3.u64 - ctx.r26.u64;
	// cmpwi cr6,r24,0
	ctx.cr6.compare<int32_t>(ctx.r24.s32, 0, ctx.xer);
	// add r10,r10,r11
	ctx.r10.u64 = ctx.r10.u64 + ctx.r11.u64;
	// stw r10,48(r31)
	REX_STORE_U32(ctx.r31.u32 + 48, ctx.r10.u32);
	// bne cr6,0x82959bd4
	if (!ctx.cr6.eq) goto loc_82959BD4;
	// lwz r10,44(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 44);
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// bne cr6,0x82959bd4
	if (!ctx.cr6.eq) goto loc_82959BD4;
	// lwz r10,56(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 56);
	// add r11,r10,r11
	ctx.r11.u64 = ctx.r10.u64 + ctx.r11.u64;
	// stw r11,56(r31)
	REX_STORE_U32(ctx.r31.u32 + 56, ctx.r11.u32);
	// b 0x82959bec
	goto loc_82959BEC;
loc_82959BD4:
	// lwz r10,60(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 60);
	// add r11,r10,r11
	ctx.r11.u64 = ctx.r10.u64 + ctx.r11.u64;
	// stw r11,60(r31)
	REX_STORE_U32(ctx.r31.u32 + 60, ctx.r11.u32);
	// b 0x82959bec
	goto loc_82959BEC;
loc_82959BE4:
	// lwz r11,76(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 76);
	// stw r11,72(r31)
	REX_STORE_U32(ctx.r31.u32 + 72, ctx.r11.u32);
loc_82959BEC:
	// lwz r3,80(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
loc_82959BF0:
	// lwz r11,252(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 252);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959c30
	if (ctx.cr6.eq) goto loc_82959C30;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x82959C08;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
	// lwz r3,80(r1)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r1.u32 + 80);
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// b 0x824131e8
	__restgprlr_24(ctx, base);
	return;
loc_82959C14:
	// lwz r11,256(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 256);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959c2c
	if (ctx.cr6.eq) goto loc_82959C2C;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x82959C2C;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
loc_82959C2C:
	// li r3,-1
	ctx.r3.s64 = -1;
loc_82959C30:
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// b 0x824131e8
	__restgprlr_24(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82959C38) {
	REX_FUNC_PROLOGUE(sub_82959C38);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// std r31,-16(r1)
	REX_STORE_U64(ctx.r1.u32 + -16, ctx.r31.u64);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// clrlwi r11,r4,31
	ctx.r11.u64 = ctx.r4.u32 & 0x1;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959cac
	if (ctx.cr6.eq) goto loc_82959CAC;
	// lwz r11,80(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 80);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x82959c6c
	if (!ctx.cr6.eq) goto loc_82959C6C;
	// li r11,1
	ctx.r11.s64 = 1;
	// stw r11,80(r31)
	REX_STORE_U32(ctx.r31.u32 + 80, ctx.r11.u32);
loc_82959C6C:
	// rlwinm r11,r4,0,0,0
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r4.u32 | (ctx.r4.u64 << 32), 0) & 0x80000000;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959ce0
	if (ctx.cr6.eq) goto loc_82959CE0;
	// lwz r11,244(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 244);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959c90
	if (ctx.cr6.eq) goto loc_82959C90;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x82959C90;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
loc_82959C90:
	// lwz r11,252(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 252);
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959ce0
	if (ctx.cr6.eq) goto loc_82959CE0;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x82959CA8;
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
	// b 0x82959ce0
	goto loc_82959CE0;
loc_82959CAC:
	// rlwinm r11,r4,0,30,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r4.u32 | (ctx.r4.u64 << 32), 0) & 0x2;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959ce0
	if (ctx.cr6.eq) goto loc_82959CE0;
	// lwz r11,80(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 80);
	// cmplwi cr6,r11,1
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 1, ctx.xer);
	// bne cr6,0x82959ccc
	if (!ctx.cr6.eq) goto loc_82959CCC;
	// li r11,0
	ctx.r11.s64 = 0;
	// stw r11,80(r31)
	REX_STORE_U32(ctx.r31.u32 + 80, ctx.r11.u32);
loc_82959CCC:
	// rlwinm r11,r4,0,0,0
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r4.u32 | (ctx.r4.u64 << 32), 0) & 0x80000000;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959ce0
	if (ctx.cr6.eq) goto loc_82959CE0;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x829599d0
	ctx.lr = 0x82959CE0;
	sub_829599D0(ctx, base);
loc_82959CE0:
	// lwz r3,80(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 80);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// ld r31,-16(r1)
	ctx.r31.u64 = REX_LOAD_U64(ctx.r1.u32 + -16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_82959CF8) {
	REX_FUNC_PROLOGUE(sub_82959CF8);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131ac
	ctx.lr = 0x82959D00;
	__savegprlr_29(ctx, base);
	// stwu r1,-112(r1)
	ea = -112 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r30,r4
	ctx.r30.u64 = ctx.r4.u64;
	// mr r29,r5
	ctx.r29.u64 = ctx.r5.u64;
	// li r5,324
	ctx.r5.s64 = 324;
	// li r4,0
	ctx.r4.s64 = 0;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// bl 0x82414530
	ctx.lr = 0x82959D1C;
	sub_82414530(ctx, base);
	// rlwinm r11,r29,0,8,8
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 0) & 0x800000;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x82959dac
	if (ctx.cr6.eq) goto loc_82959DAC;
	// li r11,1
	ctx.r11.s64 = 1;
	// stw r30,84(r31)
	REX_STORE_U32(ctx.r31.u32 + 84, ctx.r30.u32);
	// li r6,1
	ctx.r6.s64 = 1;
	// li r5,0
	ctx.r5.s64 = 0;
	// li r4,0
	ctx.r4.s64 = 0;
	// stw r11,120(r31)
	REX_STORE_U32(ctx.r31.u32 + 120, ctx.r11.u32);
	// bl 0x8240df58
	ctx.lr = 0x82959D48;
	sub_8240DF58(ctx, base);
	// stw r3,124(r31)
	REX_STORE_U32(ctx.r31.u32 + 124, ctx.r3.u32);
loc_82959D4C:
	// lis r5,-32106
	ctx.r5.s64 = -2104098816;
	// lis r6,-32106
	ctx.r6.s64 = -2104098816;
	// lis r7,-32106
	ctx.r7.s64 = -2104098816;
	// lis r8,-32106
	ctx.r8.s64 = -2104098816;
	// lis r9,-32106
	ctx.r9.s64 = -2104098816;
	// lis r10,-32106
	ctx.r10.s64 = -2104098816;
	// lis r11,-32106
	ctx.r11.s64 = -2104098816;
	// addi r5,r5,-27832
	ctx.r5.s64 = ctx.r5.s64 + -27832;
	// addi r6,r6,-27400
	ctx.r6.s64 = ctx.r6.s64 + -27400;
	// addi r7,r7,-26448
	ctx.r7.s64 = ctx.r7.s64 + -26448;
	// addi r8,r8,-26408
	ctx.r8.s64 = ctx.r8.s64 + -26408;
	// addi r9,r9,-26160
	ctx.r9.s64 = ctx.r9.s64 + -26160;
	// addi r10,r10,-26264
	ctx.r10.s64 = ctx.r10.s64 + -26264;
	// stw r5,0(r31)
	REX_STORE_U32(ctx.r31.u32 + 0, ctx.r5.u32);
	// addi r11,r11,-25544
	ctx.r11.s64 = ctx.r11.s64 + -25544;
	// stw r6,4(r31)
	REX_STORE_U32(ctx.r31.u32 + 4, ctx.r6.u32);
	// stw r7,8(r31)
	REX_STORE_U32(ctx.r31.u32 + 8, ctx.r7.u32);
	// li r3,1
	ctx.r3.s64 = 1;
	// stw r8,12(r31)
	REX_STORE_U32(ctx.r31.u32 + 12, ctx.r8.u32);
	// stw r9,16(r31)
	REX_STORE_U32(ctx.r31.u32 + 16, ctx.r9.u32);
	// stw r10,20(r31)
	REX_STORE_U32(ctx.r31.u32 + 20, ctx.r10.u32);
	// stw r11,24(r31)
	REX_STORE_U32(ctx.r31.u32 + 24, ctx.r11.u32);
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// b 0x824131fc
	__restgprlr_29(ctx, base);
	return;
loc_82959DAC:
	// lis r8,2048
	ctx.r8.s64 = 134217728;
	// li r9,0
	ctx.r9.s64 = 0;
	// ori r8,r8,128
	ctx.r8.u64 = ctx.r8.u64 | 128;
	// li r7,3
	ctx.r7.s64 = 3;
	// li r6,0
	ctx.r6.s64 = 0;
	// li r5,1
	ctx.r5.s64 = 1;
	// lis r4,-32768
	ctx.r4.s64 = -2147483648;
	// bl 0x8240e1c8
	ctx.lr = 0x82959DCC;
	sub_8240E1C8(ctx, base);
	// stw r3,84(r31)
	REX_STORE_U32(ctx.r31.u32 + 84, ctx.r3.u32);
	// lwz r11,84(r31)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 84);
	// cmpwi cr6,r11,-1
	ctx.cr6.compare<int32_t>(ctx.r11.s32, -1, ctx.xer);
	// bne cr6,0x82959d4c
	if (!ctx.cr6.eq) goto loc_82959D4C;
	// li r3,0
	ctx.r3.s64 = 0;
	// addi r1,r1,112
	ctx.r1.s64 = ctx.r1.s64 + 112;
	// b 0x824131fc
	__restgprlr_29(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_82959DE8) {
	REX_FUNC_PROLOGUE(sub_82959DE8);
	PPCRegister temp{};
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a8
	ctx.lr = 0x82959DF0;
	__savegprlr_28(ctx, base);
	// addi r12,r1,-40
	ctx.r12.s64 = ctx.r1.s64 + -40;
	// bl 0x824144c0
	ctx.lr = 0x82959DF8;
	__savefpr_26(ctx, base);
	// stwu r1,-176(r1)
	ea = -176 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r11,1
	ctx.r11.s64 = 1;
	// stw r3,0(r4)
	REX_STORE_U32(ctx.r4.u32 + 0, ctx.r3.u32);
	// mr r30,r5
	ctx.r30.u64 = ctx.r5.u64;
	// cmpwi cr6,r3,2
	ctx.cr6.compare<int32_t>(ctx.r3.s32, 2, ctx.xer);
	// stw r11,4(r4)
	REX_STORE_U32(ctx.r4.u32 + 4, ctx.r11.u32);
	// ble cr6,0x82959ff8
	if (!ctx.cr6.gt) goto loc_82959FF8;
	// srawi r31,r3,1
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0x1) != 0);
	ctx.r31.s64 = ctx.r3.s32 >> 1;
	// extsw r11,r31
	ctx.r11.s64 = ctx.r31.s32;
	// std r11,80(r1)
	REX_STORE_U64(ctx.r1.u32 + 80, ctx.r11.u64);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,6868(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 6868);
	ctx.f13.f64 = double(temp.f32);
	// lfd f0,80(r1)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r1.u32 + 80);
	// fcfid f0,f0
	ctx.f0.f64 = double(ctx.f0.s64);
	// frsp f0,f0
	ctx.f0.f64 = double(float(ctx.f0.f64));
	// fdivs f30,f13,f0
	ctx.f30.f64 = double(float(ctx.f13.f64 / ctx.f0.f64));
	// fmuls f1,f0,f30
	ctx.f1.f64 = double(float(ctx.f0.f64 * ctx.f30.f64));
	// bl 0x82413fa8
	ctx.lr = 0x82959E40;
	sub_82413FA8(ctx, base);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// frsp f26,f1
	ctx.fpscr.disableFlushMode();
	ctx.f26.f64 = double(float(ctx.f1.f64));
	// stfs f26,4(r30)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r30.u32 + 4, temp.u32);
	// cmpwi cr6,r31,4
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 4, ctx.xer);
	// lfs f27,5288(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5288);
	ctx.f27.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// stfs f27,0(r30)
	temp.f32 = float(ctx.f27.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// lfs f28,3636(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 3636);
	ctx.f28.f64 = double(temp.f32);
	// blt cr6,0x82959ea0
	if (ctx.cr6.lt) goto loc_82959EA0;
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f0,5292(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5292);
	ctx.f0.f64 = double(temp.f32);
	// fmuls f1,f30,f0
	ctx.f1.f64 = double(float(ctx.f30.f64 * ctx.f0.f64));
	// bl 0x82413fa8
	ctx.lr = 0x82959E74;
	sub_82413FA8(ctx, base);
	// fmr f13,f1
	ctx.fpscr.disableFlushMode();
	ctx.f13.f64 = ctx.f1.f64;
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f0,7880(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 7880);
	ctx.f0.f64 = double(temp.f32);
	// fmuls f1,f30,f0
	ctx.f1.f64 = double(float(ctx.f30.f64 * ctx.f0.f64));
	// frsp f0,f13
	ctx.f0.f64 = double(float(ctx.f13.f64));
	// fdivs f0,f28,f0
	ctx.f0.f64 = double(float(ctx.f28.f64 / ctx.f0.f64));
	// stfs f0,8(r30)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r30.u32 + 8, temp.u32);
	// bl 0x82413fa8
	ctx.lr = 0x82959E94;
	sub_82413FA8(ctx, base);
	// frsp f0,f1
	ctx.fpscr.disableFlushMode();
	ctx.f0.f64 = double(float(ctx.f1.f64));
	// fdivs f0,f28,f0
	ctx.f0.f64 = double(float(ctx.f28.f64 / ctx.f0.f64));
	// stfs f0,12(r30)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r30.u32 + 12, temp.u32);
loc_82959EA0:
	// li r28,4
	ctx.r28.s64 = 4;
	// cmpwi cr6,r31,4
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 4, ctx.xer);
	// ble cr6,0x82959f2c
	if (!ctx.cr6.gt) goto loc_82959F2C;
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// addi r29,r30,24
	ctx.r29.s64 = ctx.r30.s64 + 24;
	// lfs f29,8064(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 8064);
	ctx.f29.f64 = double(temp.f32);
loc_82959EB8:
	// extsw r11,r28
	ctx.r11.s64 = ctx.r28.s32;
	// std r11,80(r1)
	REX_STORE_U64(ctx.r1.u32 + 80, ctx.r11.u64);
	// lfd f0,80(r1)
	ctx.fpscr.disableFlushMode();
	ctx.f0.u64 = REX_LOAD_U64(ctx.r1.u32 + 80);
	// fcfid f0,f0
	ctx.f0.f64 = double(ctx.f0.s64);
	// frsp f0,f0
	ctx.f0.f64 = double(float(ctx.f0.f64));
	// fmuls f31,f0,f30
	ctx.f31.f64 = double(float(ctx.f0.f64 * ctx.f30.f64));
	// fmr f1,f31
	ctx.f1.f64 = ctx.f31.f64;
	// bl 0x82413fa8
	ctx.lr = 0x82959ED8;
	sub_82413FA8(ctx, base);
	// fmr f0,f1
	ctx.fpscr.disableFlushMode();
	ctx.f0.f64 = ctx.f1.f64;
	// fmr f1,f31
	ctx.f1.f64 = ctx.f31.f64;
	// frsp f0,f0
	ctx.f0.f64 = double(float(ctx.f0.f64));
	// stfs f0,-8(r29)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r29.u32 + -8, temp.u32);
	// bl 0x82413ed8
	ctx.lr = 0x82959EEC;
	sub_82413ED8(ctx, base);
	// fmuls f31,f31,f29
	ctx.fpscr.disableFlushMode();
	ctx.f31.f64 = double(float(ctx.f31.f64 * ctx.f29.f64));
	// frsp f0,f1
	ctx.f0.f64 = double(float(ctx.f1.f64));
	// stfs f0,-4(r29)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r29.u32 + -4, temp.u32);
	// fmr f1,f31
	ctx.f1.f64 = ctx.f31.f64;
	// bl 0x82413fa8
	ctx.lr = 0x82959F00;
	sub_82413FA8(ctx, base);
	// fmr f0,f1
	ctx.fpscr.disableFlushMode();
	ctx.f0.f64 = ctx.f1.f64;
	// fmr f1,f31
	ctx.f1.f64 = ctx.f31.f64;
	// frsp f0,f0
	ctx.f0.f64 = double(float(ctx.f0.f64));
	// stfs f0,0(r29)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// bl 0x82413ed8
	ctx.lr = 0x82959F14;
	sub_82413ED8(ctx, base);
	// addi r28,r28,4
	ctx.r28.s64 = ctx.r28.s64 + 4;
	// frsp f0,f1
	ctx.fpscr.disableFlushMode();
	ctx.f0.f64 = double(float(ctx.f1.f64));
	// stfs f0,4(r29)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r29.u32 + 4, temp.u32);
	// addi r29,r29,16
	ctx.r29.s64 = ctx.r29.s64 + 16;
	// cmpw cr6,r28,r31
	ctx.cr6.compare<int32_t>(ctx.r28.s32, ctx.r31.s32, ctx.xer);
	// blt cr6,0x82959eb8
	if (ctx.cr6.lt) goto loc_82959EB8;
loc_82959F2C:
	// li r11,0
	ctx.r11.s64 = 0;
	// cmpwi cr6,r31,2
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 2, ctx.xer);
	// ble cr6,0x82959ff8
	if (!ctx.cr6.gt) goto loc_82959FF8;
loc_82959F38:
	// add r8,r11,r31
	ctx.r8.u64 = ctx.r11.u64 + ctx.r31.u64;
	// srawi r31,r31,1
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x1) != 0);
	ctx.r31.s64 = ctx.r31.s32 >> 1;
	// rlwinm r10,r8,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// cmpwi cr6,r31,4
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 4, ctx.xer);
	// add r10,r10,r30
	ctx.r10.u64 = ctx.r10.u64 + ctx.r30.u64;
	// stfs f27,0(r10)
	ctx.fpscr.disableFlushMode();
	temp.f32 = float(ctx.f27.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// stfs f26,4(r10)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// blt cr6,0x82959fec
	if (ctx.cr6.lt) goto loc_82959FEC;
	// addi r10,r11,6
	ctx.r10.s64 = ctx.r11.s64 + 6;
	// addi r9,r11,4
	ctx.r9.s64 = ctx.r11.s64 + 4;
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r9,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r7,r8,2
	ctx.r7.s64 = ctx.r8.s64 + 2;
	// addi r6,r8,3
	ctx.r6.s64 = ctx.r8.s64 + 3;
	// rlwinm r7,r7,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// lfsx f0,r10,r30
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + ctx.r30.u32);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r10,r6,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// lfsx f13,r9,r30
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + ctx.r30.u32);
	ctx.f13.f64 = double(temp.f32);
	// fdivs f0,f28,f0
	ctx.f0.f64 = double(float(ctx.f28.f64 / ctx.f0.f64));
	// fdivs f13,f28,f13
	ctx.f13.f64 = double(float(ctx.f28.f64 / ctx.f13.f64));
	// stfsx f13,r7,r30
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r7.u32 + ctx.r30.u32, temp.u32);
	// stfsx f0,r10,r30
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + ctx.r30.u32, temp.u32);
	// ble cr6,0x82959fec
	if (!ctx.cr6.gt) goto loc_82959FEC;
	// addi r11,r11,10
	ctx.r11.s64 = ctx.r11.s64 + 10;
	// addi r10,r8,4
	ctx.r10.s64 = ctx.r8.s64 + 4;
	// addi r9,r31,-5
	ctx.r9.s64 = ctx.r31.s64 + -5;
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r9,30,2,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 30) & 0x3FFFFFFF;
	// add r10,r10,r30
	ctx.r10.u64 = ctx.r10.u64 + ctx.r30.u64;
	// add r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 + ctx.r30.u64;
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
loc_82959FB8:
	// addi r9,r9,-1
	ctx.r9.s64 = ctx.r9.s64 + -1;
	// lfs f0,-4(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -4);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// lfs f11,-8(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -8);
	ctx.f11.f64 = double(temp.f32);
	// addi r11,r11,32
	ctx.r11.s64 = ctx.r11.s64 + 32;
	// stfs f11,0(r10)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// stfs f0,4(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// stfs f13,8(r10)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r10.u32 + 8, temp.u32);
	// stfs f12,12(r10)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r10.u32 + 12, temp.u32);
	// addi r10,r10,16
	ctx.r10.s64 = ctx.r10.s64 + 16;
	// bne cr6,0x82959fb8
	if (!ctx.cr6.eq) goto loc_82959FB8;
loc_82959FEC:
	// mr r11,r8
	ctx.r11.u64 = ctx.r8.u64;
	// cmpwi cr6,r31,2
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 2, ctx.xer);
	// bgt cr6,0x82959f38
	if (ctx.cr6.gt) goto loc_82959F38;
loc_82959FF8:
	// addi r1,r1,176
	ctx.r1.s64 = ctx.r1.s64 + 176;
	// addi r12,r1,-40
	ctx.r12.s64 = ctx.r1.s64 + -40;
	// bl 0x8241450c
	ctx.lr = 0x8295A004;
	__restfpr_26(ctx, base);
	// b 0x824131f8
	__restgprlr_28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295A008) {
	REX_FUNC_PROLOGUE(sub_8295A008);
	PPCRegister temp{};
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a8
	ctx.lr = 0x8295A010;
	__savegprlr_28(ctx, base);
	// stfd f29,-64(r1)
	ctx.fpscr.disableFlushMode();
	REX_STORE_U64(ctx.r1.u32 + -64, ctx.f29.u64);
	// stfd f30,-56(r1)
	REX_STORE_U64(ctx.r1.u32 + -56, ctx.f30.u64);
	// stfd f31,-48(r1)
	REX_STORE_U64(ctx.r1.u32 + -48, ctx.f31.u64);
	// stwu r1,-160(r1)
	ea = -160 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r30,r3
	ctx.r30.u64 = ctx.r3.u64;
	// mr r29,r5
	ctx.r29.u64 = ctx.r5.u64;
	// cmpwi cr6,r30,1
	ctx.cr6.compare<int32_t>(ctx.r30.s32, 1, ctx.xer);
	// stw r30,4(r4)
	REX_STORE_U32(ctx.r4.u32 + 4, ctx.r30.u32);
	// ble cr6,0x8295a0f0
	if (!ctx.cr6.gt) goto loc_8295A0F0;
	// srawi r28,r30,1
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x1) != 0);
	ctx.r28.s64 = ctx.r30.s32 >> 1;
	// extsw r11,r28
	ctx.r11.s64 = ctx.r28.s32;
	// std r11,80(r1)
	REX_STORE_U64(ctx.r1.u32 + 80, ctx.r11.u64);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,6868(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 6868);
	ctx.f13.f64 = double(temp.f32);
	// lfd f0,80(r1)
	ctx.f0.u64 = REX_LOAD_U64(ctx.r1.u32 + 80);
	// fcfid f0,f0
	ctx.f0.f64 = double(ctx.f0.s64);
	// frsp f0,f0
	ctx.f0.f64 = double(float(ctx.f0.f64));
	// fdivs f29,f13,f0
	ctx.f29.f64 = double(float(ctx.f13.f64 / ctx.f0.f64));
	// fmuls f1,f0,f29
	ctx.f1.f64 = double(float(ctx.f0.f64 * ctx.f29.f64));
	// bl 0x82413fa8
	ctx.lr = 0x8295A060;
	sub_82413FA8(ctx, base);
	// frsp f0,f1
	ctx.fpscr.disableFlushMode();
	ctx.f0.f64 = double(float(ctx.f1.f64));
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// rlwinm r10,r28,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// stfs f0,0(r29)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// li r31,1
	ctx.r31.s64 = 1;
	// cmpwi cr6,r28,1
	ctx.cr6.compare<int32_t>(ctx.r28.s32, 1, ctx.xer);
	// lfs f31,3636(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 3636);
	ctx.f31.f64 = double(temp.f32);
	// fmuls f0,f0,f31
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f31.f64));
	// stfsx f0,r10,r29
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + ctx.r29.u32, temp.u32);
	// ble cr6,0x8295a0f0
	if (!ctx.cr6.gt) goto loc_8295A0F0;
	// rlwinm r11,r30,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r30,r29,4
	ctx.r30.s64 = ctx.r29.s64 + 4;
	// add r11,r11,r29
	ctx.r11.u64 = ctx.r11.u64 + ctx.r29.u64;
	// addi r29,r11,-4
	ctx.r29.s64 = ctx.r11.s64 + -4;
loc_8295A098:
	// extsw r11,r31
	ctx.r11.s64 = ctx.r31.s32;
	// std r11,80(r1)
	REX_STORE_U64(ctx.r1.u32 + 80, ctx.r11.u64);
	// lfd f0,80(r1)
	ctx.fpscr.disableFlushMode();
	ctx.f0.u64 = REX_LOAD_U64(ctx.r1.u32 + 80);
	// fcfid f0,f0
	ctx.f0.f64 = double(ctx.f0.s64);
	// frsp f0,f0
	ctx.f0.f64 = double(float(ctx.f0.f64));
	// fmuls f30,f0,f29
	ctx.f30.f64 = double(float(ctx.f0.f64 * ctx.f29.f64));
	// fmr f1,f30
	ctx.f1.f64 = ctx.f30.f64;
	// bl 0x82413fa8
	ctx.lr = 0x8295A0B8;
	sub_82413FA8(ctx, base);
	// fmr f0,f1
	ctx.fpscr.disableFlushMode();
	ctx.f0.f64 = ctx.f1.f64;
	// fmr f1,f30
	ctx.f1.f64 = ctx.f30.f64;
	// frsp f0,f0
	ctx.f0.f64 = double(float(ctx.f0.f64));
	// fmuls f0,f0,f31
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f31.f64));
	// stfs f0,0(r30)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// bl 0x82413ed8
	ctx.lr = 0x8295A0D0;
	sub_82413ED8(ctx, base);
	// frsp f0,f1
	ctx.fpscr.disableFlushMode();
	ctx.f0.f64 = double(float(ctx.f1.f64));
	// addi r31,r31,1
	ctx.r31.s64 = ctx.r31.s64 + 1;
	// addi r30,r30,4
	ctx.r30.s64 = ctx.r30.s64 + 4;
	// cmpw cr6,r31,r28
	ctx.cr6.compare<int32_t>(ctx.r31.s32, ctx.r28.s32, ctx.xer);
	// fmuls f0,f0,f31
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f31.f64));
	// stfs f0,0(r29)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// addi r29,r29,-4
	ctx.r29.s64 = ctx.r29.s64 + -4;
	// blt cr6,0x8295a098
	if (ctx.cr6.lt) goto loc_8295A098;
loc_8295A0F0:
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// lfd f29,-64(r1)
	ctx.fpscr.disableFlushMode();
	ctx.f29.u64 = REX_LOAD_U64(ctx.r1.u32 + -64);
	// lfd f30,-56(r1)
	ctx.f30.u64 = REX_LOAD_U64(ctx.r1.u32 + -56);
	// lfd f31,-48(r1)
	ctx.f31.u64 = REX_LOAD_U64(ctx.r1.u32 + -48);
	// b 0x824131f8
	__restgprlr_28(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295A108) {
	REX_FUNC_PROLOGUE(sub_8295A108);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413188
	ctx.lr = 0x8295A110;
	__savegprlr_20(ctx, base);
	// li r11,0
	ctx.r11.s64 = 0;
	// mr r7,r3
	ctx.r7.u64 = ctx.r3.u64;
	// li r20,1
	ctx.r20.s64 = 1;
	// cmpwi cr6,r7,8
	ctx.cr6.compare<int32_t>(ctx.r7.s32, 8, ctx.xer);
	// stw r11,0(r4)
	REX_STORE_U32(ctx.r4.u32 + 0, ctx.r11.u32);
	// ble cr6,0x8295a174
	if (!ctx.cr6.gt) goto loc_8295A174;
loc_8295A128:
	// cmpwi cr6,r20,0
	ctx.cr6.compare<int32_t>(ctx.r20.s32, 0, ctx.xer);
	// srawi r7,r7,1
	ctx.xer.ca = (ctx.r7.s32 < 0) & ((ctx.r7.u32 & 0x1) != 0);
	ctx.r7.s64 = ctx.r7.s32 >> 1;
	// ble cr6,0x8295a164
	if (!ctx.cr6.gt) goto loc_8295A164;
	// rlwinm r11,r20,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r20.u32 | (ctx.r20.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r10,r4
	ctx.r10.u64 = ctx.r4.u64;
	// add r9,r11,r4
	ctx.r9.u64 = ctx.r11.u64 + ctx.r4.u64;
	// mr r11,r20
	ctx.r11.u64 = ctx.r20.u64;
loc_8295A144:
	// lwz r8,0(r10)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r10.u32 + 0);
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
	// addi r10,r10,4
	ctx.r10.s64 = ctx.r10.s64 + 4;
	// add r8,r8,r7
	ctx.r8.u64 = ctx.r8.u64 + ctx.r7.u64;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// stw r8,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r8.u32);
	// addi r9,r9,4
	ctx.r9.s64 = ctx.r9.s64 + 4;
	// bne cr6,0x8295a144
	if (!ctx.cr6.eq) goto loc_8295A144;
loc_8295A164:
	// rlwinm r20,r20,1,0,30
	ctx.r20.u64 = __builtin_rotateleft64(ctx.r20.u32 | (ctx.r20.u64 << 32), 1) & 0xFFFFFFFE;
	// rlwinm r11,r20,3,0,28
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r20.u32 | (ctx.r20.u64 << 32), 3) & 0xFFFFFFF8;
	// cmpw cr6,r11,r7
	ctx.cr6.compare<int32_t>(ctx.r11.s32, ctx.r7.s32, ctx.xer);
	// blt cr6,0x8295a128
	if (ctx.cr6.lt) goto loc_8295A128;
loc_8295A174:
	// rlwinm r10,r20,3,0,28
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r20.u32 | (ctx.r20.u64 << 32), 3) & 0xFFFFFFF8;
	// rlwinm r11,r20,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r20.u32 | (ctx.r20.u64 << 32), 1) & 0xFFFFFFFE;
	// cmpw cr6,r10,r7
	ctx.cr6.compare<int32_t>(ctx.r10.s32, ctx.r7.s32, ctx.xer);
	// bne cr6,0x8295a31c
	if (!ctx.cr6.eq) goto loc_8295A31C;
	// li r21,0
	ctx.r21.s64 = 0;
	// cmpwi cr6,r20,0
	ctx.cr6.compare<int32_t>(ctx.r20.s32, 0, ctx.xer);
	// ble cr6,0x8295a630
	if (!ctx.cr6.gt) goto loc_8295A630;
	// li r22,0
	ctx.r22.s64 = 0;
	// mr r23,r4
	ctx.r23.u64 = ctx.r4.u64;
loc_8295A198:
	// cmpwi cr6,r21,0
	ctx.cr6.compare<int32_t>(ctx.r21.s32, 0, ctx.xer);
	// ble cr6,0x8295a2c4
	if (!ctx.cr6.gt) goto loc_8295A2C4;
	// rlwinm r28,r11,1,0,30
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// li r3,0
	ctx.r3.s64 = 0;
	// mr r6,r4
	ctx.r6.u64 = ctx.r4.u64;
	// mr r7,r21
	ctx.r7.u64 = ctx.r21.u64;
loc_8295A1B0:
	// lwz r9,0(r6)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// addi r7,r7,-1
	ctx.r7.s64 = ctx.r7.s64 + -1;
	// lwz r10,0(r23)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r23.u32 + 0);
	// addi r6,r6,4
	ctx.r6.s64 = ctx.r6.s64 + 4;
	// add r8,r22,r9
	ctx.r8.u64 = ctx.r22.u64 + ctx.r9.u64;
	// add r31,r3,r10
	ctx.r31.u64 = ctx.r3.u64 + ctx.r10.u64;
	// rlwinm r10,r8,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r31,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r10,r5
	ctx.r10.u64 = ctx.r10.u64 + ctx.r5.u64;
	// add r9,r9,r5
	ctx.r9.u64 = ctx.r9.u64 + ctx.r5.u64;
	// addi r30,r10,4
	ctx.r30.s64 = ctx.r10.s64 + 4;
	// addi r29,r9,4
	ctx.r29.s64 = ctx.r9.s64 + 4;
	// add r31,r31,r11
	ctx.r31.u64 = ctx.r31.u64 + ctx.r11.u64;
	// add r8,r28,r8
	ctx.r8.u64 = ctx.r28.u64 + ctx.r8.u64;
	// lfs f11,0(r10)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// rlwinm r27,r31,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f0,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r24,r8,2,0,29
	ctx.r24.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f13,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// subf r8,r11,r8
	ctx.r8.u64 = ctx.r8.u64 - ctx.r11.u64;
	// stfs f11,0(r9)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// add r31,r31,r11
	ctx.r31.u64 = ctx.r31.u64 + ctx.r11.u64;
	// lfs f12,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// add r9,r27,r5
	ctx.r9.u64 = ctx.r27.u64 + ctx.r5.u64;
	// stfs f13,0(r29)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// rlwinm r25,r31,2,0,29
	ctx.r25.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// stfs f0,0(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// add r27,r28,r8
	ctx.r27.u64 = ctx.r28.u64 + ctx.r8.u64;
	// stfs f12,0(r30)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// add r31,r31,r11
	ctx.r31.u64 = ctx.r31.u64 + ctx.r11.u64;
	// add r10,r24,r5
	ctx.r10.u64 = ctx.r24.u64 + ctx.r5.u64;
	// rlwinm r29,r27,2,0,29
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r27.u32 | (ctx.r27.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f0,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r27,r31,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r26,r8,2,0,29
	ctx.r26.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r8,r9,4
	ctx.r8.s64 = ctx.r9.s64 + 4;
	// addi r31,r10,4
	ctx.r31.s64 = ctx.r10.s64 + 4;
	// lfs f11,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// addi r3,r3,2
	ctx.r3.s64 = ctx.r3.s64 + 2;
	// cmplwi cr6,r7,0
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 0, ctx.xer);
	// lfs f13,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// stfs f12,0(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// add r8,r26,r5
	ctx.r8.u64 = ctx.r26.u64 + ctx.r5.u64;
	// stfs f11,0(r9)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// add r9,r25,r5
	ctx.r9.u64 = ctx.r25.u64 + ctx.r5.u64;
	// addi r30,r8,4
	ctx.r30.s64 = ctx.r8.s64 + 4;
	// stfs f13,0(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// stfs f0,0(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// addi r31,r9,4
	ctx.r31.s64 = ctx.r9.s64 + 4;
	// add r10,r27,r5
	ctx.r10.u64 = ctx.r27.u64 + ctx.r5.u64;
	// lfs f11,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f0,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f12,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r9)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// add r9,r29,r5
	ctx.r9.u64 = ctx.r29.u64 + ctx.r5.u64;
	// lfs f13,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// stfs f12,0(r31)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// stfs f0,0(r8)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f13,0(r30)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// lfs f0,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f11,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f13,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r10)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// stfs f12,4(r10)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// stfs f0,0(r9)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// stfs f13,4(r9)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// bne cr6,0x8295a1b0
	if (!ctx.cr6.eq) goto loc_8295A1B0;
loc_8295A2C4:
	// lwz r10,0(r23)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r23.u32 + 0);
	// addi r21,r21,1
	ctx.r21.s64 = ctx.r21.s64 + 1;
	// addi r23,r23,4
	ctx.r23.s64 = ctx.r23.s64 + 4;
	// add r10,r22,r10
	ctx.r10.u64 = ctx.r22.u64 + ctx.r10.u64;
	// addi r22,r22,2
	ctx.r22.s64 = ctx.r22.s64 + 2;
	// add r10,r10,r11
	ctx.r10.u64 = ctx.r10.u64 + ctx.r11.u64;
	// cmpw cr6,r21,r20
	ctx.cr6.compare<int32_t>(ctx.r21.s32, ctx.r20.s32, ctx.xer);
	// add r9,r10,r11
	ctx.r9.u64 = ctx.r10.u64 + ctx.r11.u64;
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r9,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r10,r5
	ctx.r10.u64 = ctx.r10.u64 + ctx.r5.u64;
	// add r9,r9,r5
	ctx.r9.u64 = ctx.r9.u64 + ctx.r5.u64;
	// lfs f0,0(r10)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f11,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f13,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r10)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// stfs f12,4(r10)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// stfs f0,0(r9)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// stfs f13,4(r9)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// blt cr6,0x8295a198
	if (ctx.cr6.lt) goto loc_8295A198;
	// b 0x824131d8
	__restgprlr_20(ctx, base);
	return;
loc_8295A31C:
	// li r25,1
	ctx.r25.s64 = 1;
	// cmpwi cr6,r20,1
	ctx.cr6.compare<int32_t>(ctx.r20.s32, 1, ctx.xer);
	// ble cr6,0x8295a630
	if (!ctx.cr6.gt) goto loc_8295A630;
	// li r3,2
	ctx.r3.s64 = 2;
	// addi r6,r4,4
	ctx.r6.s64 = ctx.r4.s64 + 4;
loc_8295A330:
	// li r26,0
	ctx.r26.s64 = 0;
	// cmpwi cr6,r25,4
	ctx.cr6.compare<int32_t>(ctx.r25.s32, 4, ctx.xer);
	// blt cr6,0x8295a570
	if (ctx.cr6.lt) goto loc_8295A570;
	// addi r10,r25,-4
	ctx.r10.s64 = ctx.r25.s64 + -4;
	// li r9,0
	ctx.r9.s64 = 0;
	// rlwinm r8,r10,30,2,31
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 30) & 0x3FFFFFFF;
	// addi r10,r4,8
	ctx.r10.s64 = ctx.r4.s64 + 8;
	// addi r31,r8,1
	ctx.r31.s64 = ctx.r8.s64 + 1;
	// rlwinm r26,r31,2,0,29
	ctx.r26.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
loc_8295A354:
	// lwz r7,-8(r10)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r10.u32 + -8);
	// lwz r8,0(r6)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// add r30,r7,r3
	ctx.r30.u64 = ctx.r7.u64 + ctx.r3.u64;
	// add r29,r8,r9
	ctx.r29.u64 = ctx.r8.u64 + ctx.r9.u64;
	// rlwinm r7,r30,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r29,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// add r7,r7,r5
	ctx.r7.u64 = ctx.r7.u64 + ctx.r5.u64;
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// add r27,r29,r11
	ctx.r27.u64 = ctx.r29.u64 + ctx.r11.u64;
	// add r28,r30,r11
	ctx.r28.u64 = ctx.r30.u64 + ctx.r11.u64;
	// addi r30,r7,4
	ctx.r30.s64 = ctx.r7.s64 + 4;
	// addi r29,r8,4
	ctx.r29.s64 = ctx.r8.s64 + 4;
	// lfs f11,0(r7)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r28,r28,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r27,r27,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r27.u32 | (ctx.r27.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f13,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r8)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// add r8,r27,r5
	ctx.r8.u64 = ctx.r27.u64 + ctx.r5.u64;
	// stfs f13,0(r29)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// add r7,r28,r5
	ctx.r7.u64 = ctx.r28.u64 + ctx.r5.u64;
	// stfs f12,0(r30)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f11,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f12,4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r8)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f12,4(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// stfs f13,4(r7)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r7.u32 + 4, temp.u32);
	// lwz r7,0(r6)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// lwz r8,-4(r10)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r10.u32 + -4);
	// add r7,r7,r9
	ctx.r7.u64 = ctx.r7.u64 + ctx.r9.u64;
	// add r8,r8,r3
	ctx.r8.u64 = ctx.r8.u64 + ctx.r3.u64;
	// addi r30,r7,2
	ctx.r30.s64 = ctx.r7.s64 + 2;
	// add r29,r8,r11
	ctx.r29.u64 = ctx.r8.u64 + ctx.r11.u64;
	// rlwinm r7,r8,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r30,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// add r7,r7,r5
	ctx.r7.u64 = ctx.r7.u64 + ctx.r5.u64;
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// rlwinm r27,r29,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// add r28,r30,r11
	ctx.r28.u64 = ctx.r30.u64 + ctx.r11.u64;
	// addi r30,r7,4
	ctx.r30.s64 = ctx.r7.s64 + 4;
	// addi r29,r8,4
	ctx.r29.s64 = ctx.r8.s64 + 4;
	// lfs f11,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r28,r28,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f13,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r8)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// add r8,r28,r5
	ctx.r8.u64 = ctx.r28.u64 + ctx.r5.u64;
	// stfs f13,0(r29)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// add r7,r27,r5
	ctx.r7.u64 = ctx.r27.u64 + ctx.r5.u64;
	// stfs f12,0(r30)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// addi r30,r9,6
	ctx.r30.s64 = ctx.r9.s64 + 6;
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f11,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f12,4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r8)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f12,4(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// stfs f13,4(r7)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r7.u32 + 4, temp.u32);
	// lwz r7,0(r6)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// lwz r8,0(r10)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r10.u32 + 0);
	// add r7,r7,r30
	ctx.r7.u64 = ctx.r7.u64 + ctx.r30.u64;
	// add r8,r8,r3
	ctx.r8.u64 = ctx.r8.u64 + ctx.r3.u64;
	// addi r29,r7,-2
	ctx.r29.s64 = ctx.r7.s64 + -2;
	// rlwinm r7,r8,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// add r28,r8,r11
	ctx.r28.u64 = ctx.r8.u64 + ctx.r11.u64;
	// add r7,r7,r5
	ctx.r7.u64 = ctx.r7.u64 + ctx.r5.u64;
	// rlwinm r8,r29,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// add r27,r29,r11
	ctx.r27.u64 = ctx.r29.u64 + ctx.r11.u64;
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// addi r29,r7,4
	ctx.r29.s64 = ctx.r7.s64 + 4;
	// lfs f11,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// rlwinm r28,r28,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r27,r27,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r27.u32 | (ctx.r27.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
	// lfs f12,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// addi r9,r9,8
	ctx.r9.s64 = ctx.r9.s64 + 8;
	// lfs f13,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// stfs f11,0(r8)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f13,4(r8)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// add r8,r27,r5
	ctx.r8.u64 = ctx.r27.u64 + ctx.r5.u64;
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// add r7,r28,r5
	ctx.r7.u64 = ctx.r28.u64 + ctx.r5.u64;
	// stfs f12,0(r29)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f11,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f13,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r8)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f12,4(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// stfs f13,4(r7)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r7.u32 + 4, temp.u32);
	// lwz r7,4(r10)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r10.u32 + 4);
	// lwz r8,0(r6)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// addi r10,r10,16
	ctx.r10.s64 = ctx.r10.s64 + 16;
	// add r29,r7,r3
	ctx.r29.u64 = ctx.r7.u64 + ctx.r3.u64;
	// add r30,r8,r30
	ctx.r30.u64 = ctx.r8.u64 + ctx.r30.u64;
	// rlwinm r7,r29,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r30,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// add r7,r7,r5
	ctx.r7.u64 = ctx.r7.u64 + ctx.r5.u64;
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// add r28,r29,r11
	ctx.r28.u64 = ctx.r29.u64 + ctx.r11.u64;
	// add r27,r30,r11
	ctx.r27.u64 = ctx.r30.u64 + ctx.r11.u64;
	// addi r30,r7,4
	ctx.r30.s64 = ctx.r7.s64 + 4;
	// addi r29,r8,4
	ctx.r29.s64 = ctx.r8.s64 + 4;
	// lfs f11,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r28,r28,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r27,r27,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r27.u32 | (ctx.r27.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f13,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r8)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// add r8,r27,r5
	ctx.r8.u64 = ctx.r27.u64 + ctx.r5.u64;
	// stfs f13,0(r29)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// add r7,r28,r5
	ctx.r7.u64 = ctx.r28.u64 + ctx.r5.u64;
	// stfs f12,0(r30)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f11,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f12,4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r8)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f12,4(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// stfs f13,4(r7)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r7.u32 + 4, temp.u32);
	// bne cr6,0x8295a354
	if (!ctx.cr6.eq) goto loc_8295A354;
loc_8295A570:
	// cmpw cr6,r26,r25
	ctx.cr6.compare<int32_t>(ctx.r26.s32, ctx.r25.s32, ctx.xer);
	// bge cr6,0x8295a61c
	if (!ctx.cr6.lt) goto loc_8295A61C;
	// rlwinm r10,r26,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r26.u32 | (ctx.r26.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r31,r26,1,0,30
	ctx.r31.u64 = __builtin_rotateleft64(ctx.r26.u32 | (ctx.r26.u64 << 32), 1) & 0xFFFFFFFE;
	// add r7,r10,r4
	ctx.r7.u64 = ctx.r10.u64 + ctx.r4.u64;
	// subf r8,r26,r25
	ctx.r8.u64 = ctx.r25.u64 - ctx.r26.u64;
loc_8295A588:
	// lwz r9,0(r7)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r7.u32 + 0);
	// addi r8,r8,-1
	ctx.r8.s64 = ctx.r8.s64 + -1;
	// lwz r10,0(r6)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// addi r7,r7,4
	ctx.r7.s64 = ctx.r7.s64 + 4;
	// add r30,r3,r9
	ctx.r30.u64 = ctx.r3.u64 + ctx.r9.u64;
	// add r29,r10,r31
	ctx.r29.u64 = ctx.r10.u64 + ctx.r31.u64;
	// rlwinm r9,r30,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r29,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r9,r5
	ctx.r9.u64 = ctx.r9.u64 + ctx.r5.u64;
	// add r10,r10,r5
	ctx.r10.u64 = ctx.r10.u64 + ctx.r5.u64;
	// add r28,r30,r11
	ctx.r28.u64 = ctx.r30.u64 + ctx.r11.u64;
	// add r27,r29,r11
	ctx.r27.u64 = ctx.r29.u64 + ctx.r11.u64;
	// addi r30,r9,4
	ctx.r30.s64 = ctx.r9.s64 + 4;
	// addi r29,r10,4
	ctx.r29.s64 = ctx.r10.s64 + 4;
	// lfs f11,0(r9)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f0,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r28,r28,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r27,r27,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r27.u32 | (ctx.r27.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r31,r31,2
	ctx.r31.s64 = ctx.r31.s64 + 2;
	// lfs f13,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// cmplwi cr6,r8,0
	ctx.cr6.compare<uint32_t>(ctx.r8.u32, 0, ctx.xer);
	// lfs f12,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r10)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// add r10,r27,r5
	ctx.r10.u64 = ctx.r27.u64 + ctx.r5.u64;
	// stfs f13,0(r29)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// stfs f0,0(r9)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// add r9,r28,r5
	ctx.r9.u64 = ctx.r28.u64 + ctx.r5.u64;
	// stfs f12,0(r30)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// lfs f0,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f11,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f12,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r10)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// stfs f12,4(r10)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// stfs f0,0(r9)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// stfs f13,4(r9)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// bne cr6,0x8295a588
	if (!ctx.cr6.eq) goto loc_8295A588;
loc_8295A61C:
	// addi r25,r25,1
	ctx.r25.s64 = ctx.r25.s64 + 1;
	// addi r6,r6,4
	ctx.r6.s64 = ctx.r6.s64 + 4;
	// addi r3,r3,2
	ctx.r3.s64 = ctx.r3.s64 + 2;
	// cmpw cr6,r25,r20
	ctx.cr6.compare<int32_t>(ctx.r25.s32, ctx.r20.s32, ctx.xer);
	// blt cr6,0x8295a330
	if (ctx.cr6.lt) goto loc_8295A330;
loc_8295A630:
	// b 0x824131d8
	__restgprlr_20(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295A638) {
	REX_FUNC_PROLOGUE(sub_8295A638);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413188
	ctx.lr = 0x8295A640;
	__savegprlr_20(ctx, base);
	// li r11,0
	ctx.r11.s64 = 0;
	// mr r7,r3
	ctx.r7.u64 = ctx.r3.u64;
	// li r20,1
	ctx.r20.s64 = 1;
	// cmpwi cr6,r7,8
	ctx.cr6.compare<int32_t>(ctx.r7.s32, 8, ctx.xer);
	// stw r11,0(r4)
	REX_STORE_U32(ctx.r4.u32 + 0, ctx.r11.u32);
	// ble cr6,0x8295a6a4
	if (!ctx.cr6.gt) goto loc_8295A6A4;
loc_8295A658:
	// cmpwi cr6,r20,0
	ctx.cr6.compare<int32_t>(ctx.r20.s32, 0, ctx.xer);
	// srawi r7,r7,1
	ctx.xer.ca = (ctx.r7.s32 < 0) & ((ctx.r7.u32 & 0x1) != 0);
	ctx.r7.s64 = ctx.r7.s32 >> 1;
	// ble cr6,0x8295a694
	if (!ctx.cr6.gt) goto loc_8295A694;
	// rlwinm r11,r20,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r20.u32 | (ctx.r20.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r10,r4
	ctx.r10.u64 = ctx.r4.u64;
	// add r9,r11,r4
	ctx.r9.u64 = ctx.r11.u64 + ctx.r4.u64;
	// mr r11,r20
	ctx.r11.u64 = ctx.r20.u64;
loc_8295A674:
	// lwz r8,0(r10)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r10.u32 + 0);
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
	// addi r10,r10,4
	ctx.r10.s64 = ctx.r10.s64 + 4;
	// add r8,r8,r7
	ctx.r8.u64 = ctx.r8.u64 + ctx.r7.u64;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// stw r8,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r8.u32);
	// addi r9,r9,4
	ctx.r9.s64 = ctx.r9.s64 + 4;
	// bne cr6,0x8295a674
	if (!ctx.cr6.eq) goto loc_8295A674;
loc_8295A694:
	// rlwinm r20,r20,1,0,30
	ctx.r20.u64 = __builtin_rotateleft64(ctx.r20.u32 | (ctx.r20.u64 << 32), 1) & 0xFFFFFFFE;
	// rlwinm r11,r20,3,0,28
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r20.u32 | (ctx.r20.u64 << 32), 3) & 0xFFFFFFF8;
	// cmpw cr6,r11,r7
	ctx.cr6.compare<int32_t>(ctx.r11.s32, ctx.r7.s32, ctx.xer);
	// blt cr6,0x8295a658
	if (ctx.cr6.lt) goto loc_8295A658;
loc_8295A6A4:
	// rlwinm r10,r20,3,0,28
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r20.u32 | (ctx.r20.u64 << 32), 3) & 0xFFFFFFF8;
	// rlwinm r11,r20,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r20.u32 | (ctx.r20.u64 << 32), 1) & 0xFFFFFFFE;
	// cmpw cr6,r10,r7
	ctx.cr6.compare<int32_t>(ctx.r10.s32, ctx.r7.s32, ctx.xer);
	// bne cr6,0x8295a8a0
	if (!ctx.cr6.eq) goto loc_8295A8A0;
	// li r21,0
	ctx.r21.s64 = 0;
	// cmpwi cr6,r20,0
	ctx.cr6.compare<int32_t>(ctx.r20.s32, 0, ctx.xer);
	// ble cr6,0x8295ac54
	if (!ctx.cr6.gt) goto loc_8295AC54;
	// li r22,0
	ctx.r22.s64 = 0;
	// mr r23,r4
	ctx.r23.u64 = ctx.r4.u64;
loc_8295A6C8:
	// cmpwi cr6,r21,0
	ctx.cr6.compare<int32_t>(ctx.r21.s32, 0, ctx.xer);
	// ble cr6,0x8295a814
	if (!ctx.cr6.gt) goto loc_8295A814;
	// rlwinm r28,r11,1,0,30
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// li r6,0
	ctx.r6.s64 = 0;
	// mr r3,r4
	ctx.r3.u64 = ctx.r4.u64;
	// mr r7,r21
	ctx.r7.u64 = ctx.r21.u64;
loc_8295A6E0:
	// lwz r9,0(r3)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);
	// addi r7,r7,-1
	ctx.r7.s64 = ctx.r7.s64 + -1;
	// lwz r10,0(r23)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r23.u32 + 0);
	// addi r3,r3,4
	ctx.r3.s64 = ctx.r3.s64 + 4;
	// add r8,r9,r22
	ctx.r8.u64 = ctx.r9.u64 + ctx.r22.u64;
	// add r31,r10,r6
	ctx.r31.u64 = ctx.r10.u64 + ctx.r6.u64;
	// rlwinm r10,r8,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r31,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r10,r5
	ctx.r10.u64 = ctx.r10.u64 + ctx.r5.u64;
	// add r9,r9,r5
	ctx.r9.u64 = ctx.r9.u64 + ctx.r5.u64;
	// addi r30,r10,4
	ctx.r30.s64 = ctx.r10.s64 + 4;
	// addi r29,r9,4
	ctx.r29.s64 = ctx.r9.s64 + 4;
	// add r31,r31,r11
	ctx.r31.u64 = ctx.r31.u64 + ctx.r11.u64;
	// add r8,r28,r8
	ctx.r8.u64 = ctx.r28.u64 + ctx.r8.u64;
	// lfs f12,0(r10)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// rlwinm r27,r31,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f0,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r24,r8,2,0,29
	ctx.r24.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f13,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// subf r8,r11,r8
	ctx.r8.u64 = ctx.r8.u64 - ctx.r11.u64;
	// stfs f12,0(r9)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// add r31,r31,r11
	ctx.r31.u64 = ctx.r31.u64 + ctx.r11.u64;
	// lfs f11,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// add r9,r27,r5
	ctx.r9.u64 = ctx.r27.u64 + ctx.r5.u64;
	// stfs f13,0(r29)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// rlwinm r25,r31,2,0,29
	ctx.r25.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// add r27,r28,r8
	ctx.r27.u64 = ctx.r28.u64 + ctx.r8.u64;
	// stfs f0,0(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// add r31,r31,r11
	ctx.r31.u64 = ctx.r31.u64 + ctx.r11.u64;
	// fneg f12,f11
	ctx.f12.u64 = ctx.f11.u64 ^ 0x8000000000000000;
	// add r10,r24,r5
	ctx.r10.u64 = ctx.r24.u64 + ctx.r5.u64;
	// stfs f12,0(r30)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// rlwinm r29,r27,2,0,29
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r27.u32 | (ctx.r27.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f0,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r27,r31,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r31,r10,4
	ctx.r31.s64 = ctx.r10.s64 + 4;
	// rlwinm r26,r8,2,0,29
	ctx.r26.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r8,r9,4
	ctx.r8.s64 = ctx.r9.s64 + 4;
	// lfs f11,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// addi r6,r6,2
	ctx.r6.s64 = ctx.r6.s64 + 2;
	// cmplwi cr6,r7,0
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 0, ctx.xer);
	// lfs f12,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// fneg f12,f12
	ctx.f12.u64 = ctx.f12.u64 ^ 0x8000000000000000;
	// stfs f11,0(r9)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// lfs f13,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// add r9,r25,r5
	ctx.r9.u64 = ctx.r25.u64 + ctx.r5.u64;
	// stfs f12,0(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// add r8,r26,r5
	ctx.r8.u64 = ctx.r26.u64 + ctx.r5.u64;
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// stfs f13,0(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// addi r30,r8,4
	ctx.r30.s64 = ctx.r8.s64 + 4;
	// stfs f0,0(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// addi r31,r9,4
	ctx.r31.s64 = ctx.r9.s64 + 4;
	// lfs f0,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// add r10,r27,r5
	ctx.r10.u64 = ctx.r27.u64 + ctx.r5.u64;
	// lfs f11,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f12,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r9)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// add r9,r29,r5
	ctx.r9.u64 = ctx.r29.u64 + ctx.r5.u64;
	// lfs f13,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// fneg f12,f12
	ctx.f12.u64 = ctx.f12.u64 ^ 0x8000000000000000;
	// stfs f12,0(r31)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// stfs f13,0(r30)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// stfs f0,0(r8)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// lfs f12,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// lfs f13,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// fneg f12,f12
	ctx.f12.u64 = ctx.f12.u64 ^ 0x8000000000000000;
	// lfs f0,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// lfs f11,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// stfs f11,0(r10)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// stfs f12,4(r10)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// stfs f0,0(r9)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// stfs f13,4(r9)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// bne cr6,0x8295a6e0
	if (!ctx.cr6.eq) goto loc_8295A6E0;
loc_8295A814:
	// lwz r10,0(r23)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r23.u32 + 0);
	// addi r21,r21,1
	ctx.r21.s64 = ctx.r21.s64 + 1;
	// addi r23,r23,4
	ctx.r23.s64 = ctx.r23.s64 + 4;
	// add r10,r10,r22
	ctx.r10.u64 = ctx.r10.u64 + ctx.r22.u64;
	// addi r22,r22,2
	ctx.r22.s64 = ctx.r22.s64 + 2;
	// add r9,r10,r11
	ctx.r9.u64 = ctx.r10.u64 + ctx.r11.u64;
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
	// add r8,r9,r11
	ctx.r8.u64 = ctx.r9.u64 + ctx.r11.u64;
	// rlwinm r7,r10,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r9,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r8,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// add r8,r8,r11
	ctx.r8.u64 = ctx.r8.u64 + ctx.r11.u64;
	// add r9,r9,r5
	ctx.r9.u64 = ctx.r9.u64 + ctx.r5.u64;
	// add r10,r10,r5
	ctx.r10.u64 = ctx.r10.u64 + ctx.r5.u64;
	// lfsx f0,r7,r5
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + ctx.r5.u32);
	ctx.f0.f64 = double(temp.f32);
	// addi r8,r8,1
	ctx.r8.s64 = ctx.r8.s64 + 1;
	// fneg f0,f0
	ctx.f0.u64 = ctx.f0.u64 ^ 0x8000000000000000;
	// stfsx f0,r7,r5
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + ctx.r5.u32, temp.u32);
	// cmpw cr6,r21,r20
	ctx.cr6.compare<int32_t>(ctx.r21.s32, ctx.r20.s32, ctx.xer);
	// rlwinm r8,r8,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f12,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// lfs f13,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// fneg f12,f12
	ctx.f12.u64 = ctx.f12.u64 ^ 0x8000000000000000;
	// lfs f0,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// lfs f11,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// stfs f11,0(r10)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// stfs f12,4(r10)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// stfs f0,0(r9)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// stfs f13,4(r9)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// lfsx f0,r8,r5
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + ctx.r5.u32);
	ctx.f0.f64 = double(temp.f32);
	// fneg f0,f0
	ctx.f0.u64 = ctx.f0.u64 ^ 0x8000000000000000;
	// stfsx f0,r8,r5
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r8.u32 + ctx.r5.u32, temp.u32);
	// blt cr6,0x8295a6c8
	if (ctx.cr6.lt) goto loc_8295A6C8;
	// b 0x824131d8
	__restgprlr_20(ctx, base);
	return;
loc_8295A8A0:
	// addi r10,r11,1
	ctx.r10.s64 = ctx.r11.s64 + 1;
	// lfs f0,4(r5)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 4);
	ctx.f0.f64 = double(temp.f32);
	// fneg f0,f0
	ctx.f0.u64 = ctx.f0.u64 ^ 0x8000000000000000;
	// stfs f0,4(r5)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r5.u32 + 4, temp.u32);
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// li r24,1
	ctx.r24.s64 = 1;
	// cmpwi cr6,r20,1
	ctx.cr6.compare<int32_t>(ctx.r20.s32, 1, ctx.xer);
	// lfsx f0,r10,r5
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + ctx.r5.u32);
	ctx.f0.f64 = double(temp.f32);
	// fneg f0,f0
	ctx.f0.u64 = ctx.f0.u64 ^ 0x8000000000000000;
	// stfsx f0,r10,r5
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + ctx.r5.u32, temp.u32);
	// ble cr6,0x8295ac54
	if (!ctx.cr6.gt) goto loc_8295AC54;
	// li r3,2
	ctx.r3.s64 = 2;
	// addi r6,r4,4
	ctx.r6.s64 = ctx.r4.s64 + 4;
loc_8295A8D4:
	// li r25,0
	ctx.r25.s64 = 0;
	// cmpwi cr6,r24,4
	ctx.cr6.compare<int32_t>(ctx.r24.s32, 4, ctx.xer);
	// blt cr6,0x8295ab50
	if (ctx.cr6.lt) goto loc_8295AB50;
	// addi r10,r24,-4
	ctx.r10.s64 = ctx.r24.s64 + -4;
	// li r9,0
	ctx.r9.s64 = 0;
	// rlwinm r8,r10,30,2,31
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 30) & 0x3FFFFFFF;
	// addi r10,r4,8
	ctx.r10.s64 = ctx.r4.s64 + 8;
	// addi r31,r8,1
	ctx.r31.s64 = ctx.r8.s64 + 1;
	// rlwinm r25,r31,2,0,29
	ctx.r25.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
loc_8295A8F8:
	// lwz r7,-8(r10)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r10.u32 + -8);
	// lwz r8,0(r6)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// add r30,r7,r3
	ctx.r30.u64 = ctx.r7.u64 + ctx.r3.u64;
	// add r29,r9,r8
	ctx.r29.u64 = ctx.r9.u64 + ctx.r8.u64;
	// rlwinm r7,r30,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r29,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// add r7,r7,r5
	ctx.r7.u64 = ctx.r7.u64 + ctx.r5.u64;
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// add r28,r30,r11
	ctx.r28.u64 = ctx.r30.u64 + ctx.r11.u64;
	// addi r30,r7,4
	ctx.r30.s64 = ctx.r7.s64 + 4;
	// add r27,r29,r11
	ctx.r27.u64 = ctx.r29.u64 + ctx.r11.u64;
	// addi r29,r8,4
	ctx.r29.s64 = ctx.r8.s64 + 4;
	// lfs f12,0(r7)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r28,r28,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r27,r27,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r27.u32 | (ctx.r27.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f13,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// lfs f11,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// stfs f12,0(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// add r8,r27,r5
	ctx.r8.u64 = ctx.r27.u64 + ctx.r5.u64;
	// stfs f13,0(r29)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// fneg f12,f11
	ctx.f12.u64 = ctx.f11.u64 ^ 0x8000000000000000;
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// add r7,r28,r5
	ctx.r7.u64 = ctx.r28.u64 + ctx.r5.u64;
	// stfs f12,0(r30)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// lfs f11,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fneg f12,f12
	ctx.f12.u64 = ctx.f12.u64 ^ 0x8000000000000000;
	// stfs f11,0(r8)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f12,4(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// stfs f13,4(r7)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r7.u32 + 4, temp.u32);
	// lwz r7,0(r6)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// lwz r8,-4(r10)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r10.u32 + -4);
	// add r7,r9,r7
	ctx.r7.u64 = ctx.r9.u64 + ctx.r7.u64;
	// add r8,r8,r3
	ctx.r8.u64 = ctx.r8.u64 + ctx.r3.u64;
	// addi r30,r7,2
	ctx.r30.s64 = ctx.r7.s64 + 2;
	// add r29,r8,r11
	ctx.r29.u64 = ctx.r8.u64 + ctx.r11.u64;
	// rlwinm r7,r8,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r30,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// add r7,r7,r5
	ctx.r7.u64 = ctx.r7.u64 + ctx.r5.u64;
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// add r28,r30,r11
	ctx.r28.u64 = ctx.r30.u64 + ctx.r11.u64;
	// addi r30,r7,4
	ctx.r30.s64 = ctx.r7.s64 + 4;
	// rlwinm r27,r29,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r29,r8,4
	ctx.r29.s64 = ctx.r8.s64 + 4;
	// lfs f12,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r28,r28,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f13,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// lfs f11,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// stfs f12,0(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// add r8,r28,r5
	ctx.r8.u64 = ctx.r28.u64 + ctx.r5.u64;
	// stfs f13,0(r29)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// fneg f12,f11
	ctx.f12.u64 = ctx.f11.u64 ^ 0x8000000000000000;
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// add r7,r27,r5
	ctx.r7.u64 = ctx.r27.u64 + ctx.r5.u64;
	// stfs f12,0(r30)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// addi r30,r9,6
	ctx.r30.s64 = ctx.r9.s64 + 6;
	// lfs f13,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// lfs f12,4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// lfs f11,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fneg f12,f12
	ctx.f12.u64 = ctx.f12.u64 ^ 0x8000000000000000;
	// stfs f11,0(r8)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f12,4(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// stfs f13,4(r7)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r7.u32 + 4, temp.u32);
	// lwz r7,0(r6)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// lwz r8,0(r10)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r10.u32 + 0);
	// add r7,r30,r7
	ctx.r7.u64 = ctx.r30.u64 + ctx.r7.u64;
	// add r8,r8,r3
	ctx.r8.u64 = ctx.r8.u64 + ctx.r3.u64;
	// addi r29,r7,-2
	ctx.r29.s64 = ctx.r7.s64 + -2;
	// rlwinm r7,r8,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// add r28,r8,r11
	ctx.r28.u64 = ctx.r8.u64 + ctx.r11.u64;
	// add r7,r7,r5
	ctx.r7.u64 = ctx.r7.u64 + ctx.r5.u64;
	// rlwinm r8,r29,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// add r27,r29,r11
	ctx.r27.u64 = ctx.r29.u64 + ctx.r11.u64;
	// addi r29,r7,4
	ctx.r29.s64 = ctx.r7.s64 + 4;
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// lfs f12,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// rlwinm r26,r28,2,0,29
	ctx.r26.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r28,r27,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r27.u32 | (ctx.r27.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
	// lfs f13,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// addi r9,r9,8
	ctx.r9.s64 = ctx.r9.s64 + 8;
	// lfs f11,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f11.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// lfs f0,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// stfs f12,0(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// fneg f12,f11
	ctx.f12.u64 = ctx.f11.u64 ^ 0x8000000000000000;
	// stfs f13,4(r8)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// add r8,r26,r5
	ctx.r8.u64 = ctx.r26.u64 + ctx.r5.u64;
	// stfs f0,0(r7)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// add r7,r28,r5
	ctx.r7.u64 = ctx.r28.u64 + ctx.r5.u64;
	// stfs f12,0(r29)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// lfs f13,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f11,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// lfs f0,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f12,4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r7)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// fneg f12,f12
	ctx.f12.u64 = ctx.f12.u64 ^ 0x8000000000000000;
	// stfs f13,4(r7)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r7.u32 + 4, temp.u32);
	// stfs f0,0(r8)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f12,4(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// lwz r8,0(r6)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// lwz r7,4(r10)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r10.u32 + 4);
	// addi r10,r10,16
	ctx.r10.s64 = ctx.r10.s64 + 16;
	// add r30,r30,r8
	ctx.r30.u64 = ctx.r30.u64 + ctx.r8.u64;
	// add r29,r7,r3
	ctx.r29.u64 = ctx.r7.u64 + ctx.r3.u64;
	// rlwinm r7,r30,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r29,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// add r7,r7,r5
	ctx.r7.u64 = ctx.r7.u64 + ctx.r5.u64;
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// add r28,r30,r11
	ctx.r28.u64 = ctx.r30.u64 + ctx.r11.u64;
	// addi r30,r7,4
	ctx.r30.s64 = ctx.r7.s64 + 4;
	// add r29,r29,r11
	ctx.r29.u64 = ctx.r29.u64 + ctx.r11.u64;
	// lfs f0,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r28,r28,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f13,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f11,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// lfs f12,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// stfs f11,0(r7)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// fneg f12,f12
	ctx.f12.u64 = ctx.f12.u64 ^ 0x8000000000000000;
	// stfs f13,0(r30)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// add r7,r28,r5
	ctx.r7.u64 = ctx.r28.u64 + ctx.r5.u64;
	// stfs f0,0(r8)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f12,4(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// rlwinm r8,r29,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// lfs f13,4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f0,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// lfs f12,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// lfs f11,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fneg f12,f12
	ctx.f12.u64 = ctx.f12.u64 ^ 0x8000000000000000;
	// stfs f11,0(r7)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// stfs f12,4(r7)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r7.u32 + 4, temp.u32);
	// stfs f0,0(r8)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f13,4(r8)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// bne cr6,0x8295a8f8
	if (!ctx.cr6.eq) goto loc_8295A8F8;
loc_8295AB50:
	// cmpw cr6,r25,r24
	ctx.cr6.compare<int32_t>(ctx.r25.s32, ctx.r24.s32, ctx.xer);
	// bge cr6,0x8295ac0c
	if (!ctx.cr6.lt) goto loc_8295AC0C;
	// rlwinm r10,r25,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r25.u32 | (ctx.r25.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r31,r25,1,0,30
	ctx.r31.u64 = __builtin_rotateleft64(ctx.r25.u32 | (ctx.r25.u64 << 32), 1) & 0xFFFFFFFE;
	// add r7,r10,r4
	ctx.r7.u64 = ctx.r10.u64 + ctx.r4.u64;
	// subf r8,r25,r24
	ctx.r8.u64 = ctx.r24.u64 - ctx.r25.u64;
loc_8295AB68:
	// lwz r9,0(r7)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r7.u32 + 0);
	// addi r8,r8,-1
	ctx.r8.s64 = ctx.r8.s64 + -1;
	// lwz r10,0(r6)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// addi r7,r7,4
	ctx.r7.s64 = ctx.r7.s64 + 4;
	// add r30,r3,r9
	ctx.r30.u64 = ctx.r3.u64 + ctx.r9.u64;
	// add r29,r31,r10
	ctx.r29.u64 = ctx.r31.u64 + ctx.r10.u64;
	// rlwinm r9,r30,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r29,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r9,r5
	ctx.r9.u64 = ctx.r9.u64 + ctx.r5.u64;
	// add r10,r10,r5
	ctx.r10.u64 = ctx.r10.u64 + ctx.r5.u64;
	// add r28,r30,r11
	ctx.r28.u64 = ctx.r30.u64 + ctx.r11.u64;
	// addi r30,r9,4
	ctx.r30.s64 = ctx.r9.s64 + 4;
	// add r27,r29,r11
	ctx.r27.u64 = ctx.r29.u64 + ctx.r11.u64;
	// addi r29,r10,4
	ctx.r29.s64 = ctx.r10.s64 + 4;
	// lfs f12,0(r9)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// lfs f0,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r28,r28,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r27,r27,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r27.u32 | (ctx.r27.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f13,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// addi r31,r31,2
	ctx.r31.s64 = ctx.r31.s64 + 2;
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// stfs f12,0(r10)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// lfs f11,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// add r10,r27,r5
	ctx.r10.u64 = ctx.r27.u64 + ctx.r5.u64;
	// stfs f13,0(r29)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// fneg f12,f11
	ctx.f12.u64 = ctx.f11.u64 ^ 0x8000000000000000;
	// stfs f0,0(r9)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// add r9,r28,r5
	ctx.r9.u64 = ctx.r28.u64 + ctx.r5.u64;
	// stfs f12,0(r30)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// cmplwi cr6,r8,0
	ctx.cr6.compare<uint32_t>(ctx.r8.u32, 0, ctx.xer);
	// lfs f13,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f0,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// fneg f13,f13
	ctx.f13.u64 = ctx.f13.u64 ^ 0x8000000000000000;
	// lfs f12,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// lfs f11,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fneg f12,f12
	ctx.f12.u64 = ctx.f12.u64 ^ 0x8000000000000000;
	// stfs f11,0(r10)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// stfs f12,4(r10)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// stfs f0,0(r9)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// stfs f13,4(r9)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// bne cr6,0x8295ab68
	if (!ctx.cr6.eq) goto loc_8295AB68;
loc_8295AC0C:
	// lwz r10,0(r6)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// addi r24,r24,1
	ctx.r24.s64 = ctx.r24.s64 + 1;
	// addi r6,r6,4
	ctx.r6.s64 = ctx.r6.s64 + 4;
	// add r10,r3,r10
	ctx.r10.u64 = ctx.r3.u64 + ctx.r10.u64;
	// addi r3,r3,2
	ctx.r3.s64 = ctx.r3.s64 + 2;
	// addi r9,r10,1
	ctx.r9.s64 = ctx.r10.s64 + 1;
	// add r10,r10,r11
	ctx.r10.u64 = ctx.r10.u64 + ctx.r11.u64;
	// rlwinm r9,r9,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
	// cmpw cr6,r24,r20
	ctx.cr6.compare<int32_t>(ctx.r24.s32, ctx.r20.s32, ctx.xer);
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// lfsx f0,r9,r5
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + ctx.r5.u32);
	ctx.f0.f64 = double(temp.f32);
	// fneg f0,f0
	ctx.f0.u64 = ctx.f0.u64 ^ 0x8000000000000000;
	// stfsx f0,r9,r5
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r9.u32 + ctx.r5.u32, temp.u32);
	// lfsx f0,r10,r5
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + ctx.r5.u32);
	ctx.f0.f64 = double(temp.f32);
	// fneg f0,f0
	ctx.f0.u64 = ctx.f0.u64 ^ 0x8000000000000000;
	// stfsx f0,r10,r5
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + ctx.r5.u32, temp.u32);
	// blt cr6,0x8295a8d4
	if (ctx.cr6.lt) goto loc_8295A8D4;
loc_8295AC54:
	// b 0x824131d8
	__restgprlr_20(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295AC58) {
	REX_FUNC_PROLOGUE(sub_8295AC58);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// addi r12,r1,-8
	ctx.r12.s64 = ctx.r1.s64 + -8;
	// bl 0x8241449c
	ctx.lr = 0x8295AC68;
	__savefpr_17(ctx, base);
	// lfs f0,8(r3)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 8);
	ctx.f0.f64 = double(temp.f32);
	// lfs f2,64(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 64);
	ctx.f2.f64 = double(temp.f32);
	// stfs f0,64(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 64, temp.u32);
	// lfs f13,12(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 12);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,24(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 24);
	ctx.f12.f64 = double(temp.f32);
	// lfs f11,28(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 28);
	ctx.f11.f64 = double(temp.f32);
	// lfs f10,40(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 40);
	ctx.f10.f64 = double(temp.f32);
	// lfs f9,44(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 44);
	ctx.f9.f64 = double(temp.f32);
	// lfs f8,16(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 16);
	ctx.f8.f64 = double(temp.f32);
	// lfs f7,20(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 20);
	ctx.f7.f64 = double(temp.f32);
	// lfs f6,32(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 32);
	ctx.f6.f64 = double(temp.f32);
	// lfs f5,36(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 36);
	ctx.f5.f64 = double(temp.f32);
	// lfs f4,48(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 48);
	ctx.f4.f64 = double(temp.f32);
	// lfs f3,52(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 52);
	ctx.f3.f64 = double(temp.f32);
	// lfs f1,68(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 68);
	ctx.f1.f64 = double(temp.f32);
	// lfs f27,56(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 56);
	ctx.f27.f64 = double(temp.f32);
	// lfs f26,60(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 60);
	ctx.f26.f64 = double(temp.f32);
	// lfs f25,72(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 72);
	ctx.f25.f64 = double(temp.f32);
	// lfs f24,120(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 120);
	ctx.f24.f64 = double(temp.f32);
	// lfs f23,124(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 124);
	ctx.f23.f64 = double(temp.f32);
	// lfs f22,88(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 88);
	ctx.f22.f64 = double(temp.f32);
	// lfs f21,92(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 92);
	ctx.f21.f64 = double(temp.f32);
	// lfs f20,104(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 104);
	ctx.f20.f64 = double(temp.f32);
	// lfs f19,108(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 108);
	ctx.f19.f64 = double(temp.f32);
	// lfs f18,76(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 76);
	ctx.f18.f64 = double(temp.f32);
	// lfs f17,112(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 112);
	ctx.f17.f64 = double(temp.f32);
	// lfs f31,80(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 80);
	ctx.f31.f64 = double(temp.f32);
	// lfs f30,84(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 84);
	ctx.f30.f64 = double(temp.f32);
	// lfs f29,96(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 96);
	ctx.f29.f64 = double(temp.f32);
	// lfs f28,100(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 100);
	ctx.f28.f64 = double(temp.f32);
	// lfs f0,116(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 116);
	ctx.f0.f64 = double(temp.f32);
	// stfs f24,8(r3)
	temp.f32 = float(ctx.f24.f64);
	REX_STORE_U32(ctx.r3.u32 + 8, temp.u32);
	// stfs f23,12(r3)
	temp.f32 = float(ctx.f23.f64);
	REX_STORE_U32(ctx.r3.u32 + 12, temp.u32);
	// stfs f27,16(r3)
	temp.f32 = float(ctx.f27.f64);
	REX_STORE_U32(ctx.r3.u32 + 16, temp.u32);
	// stfs f26,20(r3)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r3.u32 + 20, temp.u32);
	// stfs f22,24(r3)
	temp.f32 = float(ctx.f22.f64);
	REX_STORE_U32(ctx.r3.u32 + 24, temp.u32);
	// stfs f21,28(r3)
	temp.f32 = float(ctx.f21.f64);
	REX_STORE_U32(ctx.r3.u32 + 28, temp.u32);
	// stfs f20,40(r3)
	temp.f32 = float(ctx.f20.f64);
	REX_STORE_U32(ctx.r3.u32 + 40, temp.u32);
	// stfs f19,44(r3)
	temp.f32 = float(ctx.f19.f64);
	REX_STORE_U32(ctx.r3.u32 + 44, temp.u32);
	// stfs f25,56(r3)
	temp.f32 = float(ctx.f25.f64);
	REX_STORE_U32(ctx.r3.u32 + 56, temp.u32);
	// stfs f18,60(r3)
	temp.f32 = float(ctx.f18.f64);
	REX_STORE_U32(ctx.r3.u32 + 60, temp.u32);
	// stfs f17,72(r3)
	temp.f32 = float(ctx.f17.f64);
	REX_STORE_U32(ctx.r3.u32 + 72, temp.u32);
	// stfs f13,68(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 68, temp.u32);
	// stfs f12,32(r3)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r3.u32 + 32, temp.u32);
	// stfs f11,36(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 36, temp.u32);
	// stfs f10,48(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 48, temp.u32);
	// stfs f9,52(r3)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r3.u32 + 52, temp.u32);
	// stfs f4,80(r3)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r3.u32 + 80, temp.u32);
	// stfs f3,84(r3)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r3.u32 + 84, temp.u32);
	// stfs f31,88(r3)
	temp.f32 = float(ctx.f31.f64);
	REX_STORE_U32(ctx.r3.u32 + 88, temp.u32);
	// stfs f30,92(r3)
	temp.f32 = float(ctx.f30.f64);
	REX_STORE_U32(ctx.r3.u32 + 92, temp.u32);
	// stfs f8,96(r3)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r3.u32 + 96, temp.u32);
	// stfs f7,100(r3)
	temp.f32 = float(ctx.f7.f64);
	REX_STORE_U32(ctx.r3.u32 + 100, temp.u32);
	// stfs f29,104(r3)
	temp.f32 = float(ctx.f29.f64);
	REX_STORE_U32(ctx.r3.u32 + 104, temp.u32);
	// stfs f28,108(r3)
	temp.f32 = float(ctx.f28.f64);
	REX_STORE_U32(ctx.r3.u32 + 108, temp.u32);
	// stfs f6,112(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 112, temp.u32);
	// stfs f2,120(r3)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r3.u32 + 120, temp.u32);
	// stfs f1,124(r3)
	temp.f32 = float(ctx.f1.f64);
	REX_STORE_U32(ctx.r3.u32 + 124, temp.u32);
	// stfs f0,76(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 76, temp.u32);
	// stfs f5,116(r3)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r3.u32 + 116, temp.u32);
	// addi r12,r1,-8
	ctx.r12.s64 = ctx.r1.s64 + -8;
	// bl 0x824144e8
	ctx.lr = 0x8295AD60;
	__restfpr_17(ctx, base);
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_8295AD70) {
	REX_FUNC_PROLOGUE(sub_8295AD70);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x8241318c
	ctx.lr = 0x8295AD78;
	__savegprlr_21(ctx, base);
	// addi r12,r1,-96
	ctx.r12.s64 = ctx.r1.s64 + -96;
	// bl 0x82414490
	ctx.lr = 0x8295AD80;
	__savefpr_14(ctx, base);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f10,0(r4)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 0);
	ctx.f10.f64 = double(temp.f32);
	// srawi r28,r3,3
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0x7) != 0);
	ctx.r28.s64 = ctx.r3.s32 >> 3;
	// lfs f9,4(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 4);
	ctx.f9.f64 = double(temp.f32);
	// addi r30,r5,8
	ctx.r30.s64 = ctx.r5.s64 + 8;
	// rlwinm r3,r28,1,0,30
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 1) & 0xFFFFFFFE;
	// addi r21,r28,-2
	ctx.r21.s64 = ctx.r28.s64 + -2;
	// lfs f0,5288(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5288);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// rlwinm r10,r3,1,0,30
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 1) & 0xFFFFFFFE;
	// fmr f12,f0
	ctx.f12.f64 = ctx.f0.f64;
	// cmpwi cr6,r21,2
	ctx.cr6.compare<int32_t>(ctx.r21.s32, 2, ctx.xer);
	// add r9,r10,r3
	ctx.r9.u64 = ctx.r10.u64 + ctx.r3.u64;
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f13,4712(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4712);
	ctx.f13.f64 = double(temp.f32);
	// rlwinm r9,r9,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r11,r3,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 2) & 0xFFFFFFFC;
	// fmr f11,f13
	ctx.f11.f64 = ctx.f13.f64;
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// add r11,r11,r4
	ctx.r11.u64 = ctx.r11.u64 + ctx.r4.u64;
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// lfs f7,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f7.f64 = double(temp.f32);
	// lfs f4,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f4.f64 = double(temp.f32);
	// fadds f3,f10,f7
	ctx.f3.f64 = double(float(ctx.f10.f64 + ctx.f7.f64));
	// lfs f5,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f5.f64 = double(temp.f32);
	// fadds f1,f9,f4
	ctx.f1.f64 = double(float(ctx.f9.f64 + ctx.f4.f64));
	// lfs f8,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f10,f10,f7
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f7.f64));
	// fadds f2,f8,f5
	ctx.f2.f64 = double(float(ctx.f8.f64 + ctx.f5.f64));
	// lfs f6,4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4);
	ctx.f6.f64 = double(temp.f32);
	// lfs f7,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f7.f64 = double(temp.f32);
	// fsubs f8,f8,f5
	ctx.f8.f64 = double(float(ctx.f8.f64 - ctx.f5.f64));
	// fsubs f9,f9,f4
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f4.f64));
	// fadds f5,f6,f7
	ctx.f5.f64 = double(float(ctx.f6.f64 + ctx.f7.f64));
	// fsubs f7,f6,f7
	ctx.f7.f64 = double(float(ctx.f6.f64 - ctx.f7.f64));
	// fadds f6,f2,f3
	ctx.f6.f64 = double(float(ctx.f2.f64 + ctx.f3.f64));
	// stfs f6,0(r4)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r4.u32 + 0, temp.u32);
	// fsubs f6,f3,f2
	ctx.f6.f64 = double(float(ctx.f3.f64 - ctx.f2.f64));
	// fadds f4,f8,f9
	ctx.f4.f64 = double(float(ctx.f8.f64 + ctx.f9.f64));
	// fsubs f9,f9,f8
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f8.f64));
	// fadds f8,f5,f1
	ctx.f8.f64 = double(float(ctx.f5.f64 + ctx.f1.f64));
	// stfs f8,4(r4)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r4.u32 + 4, temp.u32);
	// fsubs f8,f1,f5
	ctx.f8.f64 = double(float(ctx.f1.f64 - ctx.f5.f64));
	// stfs f8,4(r11)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r11.u32 + 4, temp.u32);
	// stfs f6,0(r11)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// fsubs f8,f10,f7
	ctx.f8.f64 = double(float(ctx.f10.f64 - ctx.f7.f64));
	// stfs f8,0(r10)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// fadds f10,f7,f10
	ctx.f10.f64 = double(float(ctx.f7.f64 + ctx.f10.f64));
	// stfs f4,4(r10)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// stfs f10,0(r9)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// stfs f9,4(r9)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// lfs f15,4(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 4);
	ctx.f15.f64 = double(temp.f32);
	// lfs f17,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f17.f64 = double(temp.f32);
	// lfs f16,12(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 12);
	ctx.f16.f64 = double(temp.f32);
	// ble cr6,0x8295b1f0
	if (!ctx.cr6.gt) goto loc_8295B1F0;
	// addi r10,r3,4
	ctx.r10.s64 = ctx.r3.s64 + 4;
	// addi r6,r3,-2
	ctx.r6.s64 = ctx.r3.s64 + -2;
	// addi r7,r3,2
	ctx.r7.s64 = ctx.r3.s64 + 2;
	// rlwinm r11,r3,4,0,27
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 4) & 0xFFFFFFF0;
	// addi r31,r21,-3
	ctx.r31.s64 = ctx.r21.s64 + -3;
	// rlwinm r9,r10,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r6,r6,3,0,28
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 3) & 0xFFFFFFF8;
	// add r11,r11,r4
	ctx.r11.u64 = ctx.r11.u64 + ctx.r4.u64;
	// rlwinm r10,r7,3,0,28
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 3) & 0xFFFFFFF8;
	// rlwinm r7,r31,30,2,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 30) & 0x3FFFFFFF;
	// add r31,r6,r4
	ctx.r31.u64 = ctx.r6.u64 + ctx.r4.u64;
	// addi r6,r11,-16
	ctx.r6.s64 = ctx.r11.s64 + -16;
	// rlwinm r11,r3,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 1) & 0xFFFFFFFE;
	// addi r8,r3,-4
	ctx.r8.s64 = ctx.r3.s64 + -4;
	// add r11,r3,r11
	ctx.r11.u64 = ctx.r3.u64 + ctx.r11.u64;
	// addi r29,r7,1
	ctx.r29.s64 = ctx.r7.s64 + 1;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r5,r8,2,0,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// add r7,r11,r4
	ctx.r7.u64 = ctx.r11.u64 + ctx.r4.u64;
	// addi r8,r4,16
	ctx.r8.s64 = ctx.r4.s64 + 16;
	// addi r11,r7,16
	ctx.r11.s64 = ctx.r7.s64 + 16;
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// add r5,r5,r4
	ctx.r5.u64 = ctx.r5.u64 + ctx.r4.u64;
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// addi r7,r7,-16
	ctx.r7.s64 = ctx.r7.s64 + -16;
loc_8295AEC0:
	// addi r30,r30,16
	ctx.r30.s64 = ctx.r30.s64 + 16;
	// lfs f9,0(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f9.f64 = double(temp.f32);
	// lfs f7,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f7.f64 = double(temp.f32);
	// addi r27,r5,12
	ctx.r27.s64 = ctx.r5.s64 + 12;
	// lfs f10,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f10.f64 = double(temp.f32);
	// fadds f1,f7,f9
	ctx.f1.f64 = double(float(ctx.f7.f64 + ctx.f9.f64));
	// lfs f8,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f29,f7,f9
	ctx.f29.f64 = double(float(ctx.f7.f64 - ctx.f9.f64));
	// fadds f3,f10,f8
	ctx.f3.f64 = double(float(ctx.f10.f64 + ctx.f8.f64));
	// lfs f4,-8(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -8);
	ctx.f4.f64 = double(temp.f32);
	// lfs f9,-8(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + -8);
	ctx.f9.f64 = double(temp.f32);
	// fsubs f30,f8,f10
	ctx.f30.f64 = double(float(ctx.f8.f64 - ctx.f10.f64));
	// fadds f0,f9,f0
	ctx.f0.f64 = double(float(ctx.f9.f64 + ctx.f0.f64));
	// lfs f6,-8(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + -8);
	ctx.f6.f64 = double(temp.f32);
	// lfs f10,-4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -4);
	ctx.f10.f64 = double(temp.f32);
	// fadds f21,f6,f4
	ctx.f21.f64 = double(float(ctx.f6.f64 + ctx.f4.f64));
	// lfs f31,-4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -4);
	ctx.f31.f64 = double(temp.f32);
	// fsubs f6,f6,f4
	ctx.f6.f64 = double(float(ctx.f6.f64 - ctx.f4.f64));
	// fsubs f4,f31,f10
	ctx.f4.f64 = double(float(ctx.f31.f64 - ctx.f10.f64));
	// lfs f2,-8(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -8);
	ctx.f2.f64 = double(temp.f32);
	// lfs f5,-8(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -8);
	ctx.f5.f64 = double(temp.f32);
	// fadds f31,f31,f10
	ctx.f31.f64 = double(float(ctx.f31.f64 + ctx.f10.f64));
	// lfs f26,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f26.f64 = double(temp.f32);
	// fadds f20,f5,f2
	ctx.f20.f64 = double(float(ctx.f5.f64 + ctx.f2.f64));
	// lfs f25,4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4);
	ctx.f25.f64 = double(temp.f32);
	// fsubs f5,f5,f2
	ctx.f5.f64 = double(float(ctx.f5.f64 - ctx.f2.f64));
	// lfs f24,-4(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + -4);
	ctx.f24.f64 = double(temp.f32);
	// addi r26,r5,4
	ctx.r26.s64 = ctx.r5.s64 + 4;
	// lfs f27,-4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -4);
	ctx.f27.f64 = double(temp.f32);
	// fadds f10,f1,f3
	ctx.f10.f64 = double(float(ctx.f1.f64 + ctx.f3.f64));
	// lfs f23,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f23.f64 = double(temp.f32);
	// addi r25,r7,4
	ctx.r25.s64 = ctx.r7.s64 + 4;
	// lfs f22,4(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 4);
	ctx.f22.f64 = double(temp.f32);
	// fadds f14,f23,f12
	ctx.f14.f64 = double(float(ctx.f23.f64 + ctx.f12.f64));
	// lfs f28,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f28.f64 = double(temp.f32);
	// fmr f12,f23
	ctx.f12.f64 = ctx.f23.f64;
	// lfs f8,-4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + -4);
	ctx.f8.f64 = double(temp.f32);
	// fadds f18,f28,f25
	ctx.f18.f64 = double(float(ctx.f28.f64 + ctx.f25.f64));
	// stfs f10,0(r8)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// fmuls f10,f0,f17
	ctx.f10.f64 = double(float(ctx.f0.f64 * ctx.f17.f64));
	// fadds f2,f8,f27
	ctx.f2.f64 = double(float(ctx.f8.f64 + ctx.f27.f64));
	// lfs f7,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f7.f64 = double(temp.f32);
	// fsubs f0,f11,f22
	ctx.f0.f64 = double(float(ctx.f11.f64 - ctx.f22.f64));
	// stfs f0,-256(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -256, temp.u32);
	// fsubs f27,f8,f27
	ctx.f27.f64 = double(float(ctx.f8.f64 - ctx.f27.f64));
	// addi r24,r31,12
	ctx.r24.s64 = ctx.r31.s64 + 12;
	// fadds f8,f24,f13
	ctx.f8.f64 = double(float(ctx.f24.f64 + ctx.f13.f64));
	// fsubs f13,f3,f1
	ctx.f13.f64 = double(float(ctx.f3.f64 - ctx.f1.f64));
	// stfs f13,-252(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -252, temp.u32);
	// fadds f19,f7,f26
	ctx.f19.f64 = double(float(ctx.f7.f64 + ctx.f26.f64));
	// fsubs f1,f28,f25
	ctx.f1.f64 = double(float(ctx.f28.f64 - ctx.f25.f64));
	// fadds f28,f20,f21
	ctx.f28.f64 = double(float(ctx.f20.f64 + ctx.f21.f64));
	// stfs f28,-8(r8)
	temp.f32 = float(ctx.f28.f64);
	REX_STORE_U32(ctx.r8.u32 + -8, temp.u32);
	// fmr f0,f9
	ctx.f0.f64 = ctx.f9.f64;
	// fsubs f3,f7,f26
	ctx.f3.f64 = double(float(ctx.f7.f64 - ctx.f26.f64));
	// fadds f25,f31,f2
	ctx.f25.f64 = double(float(ctx.f31.f64 + ctx.f2.f64));
	// stfs f25,-4(r8)
	temp.f32 = float(ctx.f25.f64);
	REX_STORE_U32(ctx.r8.u32 + -4, temp.u32);
	// fsubs f2,f2,f31
	ctx.f2.f64 = double(float(ctx.f2.f64 - ctx.f31.f64));
	// fsubs f26,f21,f20
	ctx.f26.f64 = double(float(ctx.f21.f64 - ctx.f20.f64));
	// fmuls f9,f8,f17
	ctx.f9.f64 = double(float(ctx.f8.f64 * ctx.f17.f64));
	// fsubs f28,f6,f4
	ctx.f28.f64 = double(float(ctx.f6.f64 - ctx.f4.f64));
	// fadds f31,f18,f19
	ctx.f31.f64 = double(float(ctx.f18.f64 + ctx.f19.f64));
	// stfs f31,4(r8)
	temp.f32 = float(ctx.f31.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// stfs f2,-4(r9)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r9.u32 + -4, temp.u32);
	// fadds f2,f5,f27
	ctx.f2.f64 = double(float(ctx.f5.f64 + ctx.f27.f64));
	// stfs f26,-8(r9)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r9.u32 + -8, temp.u32);
	// fmuls f8,f14,f16
	ctx.f8.f64 = double(float(ctx.f14.f64 * ctx.f16.f64));
	// fsubs f31,f19,f18
	ctx.f31.f64 = double(float(ctx.f19.f64 - ctx.f18.f64));
	// stfs f31,4(r9)
	temp.f32 = float(ctx.f31.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// fmr f13,f24
	ctx.f13.f64 = ctx.f24.f64;
	// fneg f11,f22
	ctx.f11.u64 = ctx.f22.u64 ^ 0x8000000000000000;
	// fadds f31,f29,f3
	ctx.f31.f64 = double(float(ctx.f29.f64 + ctx.f3.f64));
	// fmuls f24,f9,f28
	ctx.f24.f64 = double(float(ctx.f9.f64 * ctx.f28.f64));
	// fmuls f26,f9,f2
	ctx.f26.f64 = double(float(ctx.f9.f64 * ctx.f2.f64));
	// fmr f25,f2
	ctx.f25.f64 = ctx.f2.f64;
	// fsubs f2,f30,f1
	ctx.f2.f64 = double(float(ctx.f30.f64 - ctx.f1.f64));
	// fmsubs f28,f10,f28,f26
	ctx.f28.f64 = double(float(std::fma(ctx.f10.f64, ctx.f28.f64, -ctx.f26.f64)));
	// lfs f14,-256(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -256);
	ctx.f14.f64 = double(temp.f32);
	// fmuls f7,f14,f16
	ctx.f7.f64 = double(float(ctx.f14.f64 * ctx.f16.f64));
	// lfs f14,-252(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -252);
	ctx.f14.f64 = double(temp.f32);
	// stfs f14,0(r9)
	temp.f32 = float(ctx.f14.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// stfs f28,-8(r10)
	temp.f32 = float(ctx.f28.f64);
	REX_STORE_U32(ctx.r10.u32 + -8, temp.u32);
	// fmadds f28,f10,f25,f24
	ctx.f28.f64 = double(float(std::fma(ctx.f10.f64, ctx.f25.f64, ctx.f24.f64)));
	// stfs f28,-4(r10)
	temp.f32 = float(ctx.f28.f64);
	REX_STORE_U32(ctx.r10.u32 + -4, temp.u32);
	// fmuls f28,f13,f31
	ctx.f28.f64 = double(float(ctx.f13.f64 * ctx.f31.f64));
	// fmuls f26,f13,f2
	ctx.f26.f64 = double(float(ctx.f13.f64 * ctx.f2.f64));
	// addi r23,r6,4
	ctx.r23.s64 = ctx.r6.s64 + 4;
	// fadds f6,f4,f6
	ctx.f6.f64 = double(float(ctx.f4.f64 + ctx.f6.f64));
	// addi r22,r31,4
	ctx.r22.s64 = ctx.r31.s64 + 4;
	// fsubs f5,f27,f5
	ctx.f5.f64 = double(float(ctx.f27.f64 - ctx.f5.f64));
	// addi r29,r29,-1
	ctx.r29.s64 = ctx.r29.s64 + -1;
	// addi r9,r9,16
	ctx.r9.s64 = ctx.r9.s64 + 16;
	// addi r8,r8,16
	ctx.r8.s64 = ctx.r8.s64 + 16;
	// cmplwi cr6,r29,0
	ctx.cr6.compare<uint32_t>(ctx.r29.u32, 0, ctx.xer);
	// fmsubs f4,f0,f2,f28
	ctx.f4.f64 = double(float(std::fma(ctx.f0.f64, ctx.f2.f64, -ctx.f28.f64)));
	// stfs f4,0(r10)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// fmadds f4,f0,f31,f26
	ctx.f4.f64 = double(float(std::fma(ctx.f0.f64, ctx.f31.f64, ctx.f26.f64)));
	// stfs f4,4(r10)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// fmuls f4,f8,f6
	ctx.f4.f64 = double(float(ctx.f8.f64 * ctx.f6.f64));
	// addi r10,r10,16
	ctx.r10.s64 = ctx.r10.s64 + 16;
	// fmr f2,f5
	ctx.f2.f64 = ctx.f5.f64;
	// fmuls f28,f7,f6
	ctx.f28.f64 = double(float(ctx.f7.f64 * ctx.f6.f64));
	// fmr f31,f5
	ctx.f31.f64 = ctx.f5.f64;
	// fadds f6,f1,f30
	ctx.f6.f64 = double(float(ctx.f1.f64 + ctx.f30.f64));
	// fsubs f5,f3,f29
	ctx.f5.f64 = double(float(ctx.f3.f64 - ctx.f29.f64));
	// fmadds f4,f7,f2,f4
	ctx.f4.f64 = double(float(std::fma(ctx.f7.f64, ctx.f2.f64, ctx.f4.f64)));
	// stfs f4,-8(r11)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r11.u32 + -8, temp.u32);
	// fmsubs f4,f8,f31,f28
	ctx.f4.f64 = double(float(std::fma(ctx.f8.f64, ctx.f31.f64, -ctx.f28.f64)));
	// stfs f4,-4(r11)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r11.u32 + -4, temp.u32);
	// fmuls f4,f12,f6
	ctx.f4.f64 = double(float(ctx.f12.f64 * ctx.f6.f64));
	// fmuls f6,f11,f6
	ctx.f6.f64 = double(float(ctx.f11.f64 * ctx.f6.f64));
	// fmadds f4,f11,f5,f4
	ctx.f4.f64 = double(float(std::fma(ctx.f11.f64, ctx.f5.f64, ctx.f4.f64)));
	// stfs f4,0(r11)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// fmsubs f6,f12,f5,f6
	ctx.f6.f64 = double(float(std::fma(ctx.f12.f64, ctx.f5.f64, -ctx.f6.f64)));
	// stfs f6,4(r11)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r11.u32 + 4, temp.u32);
	// lfs f5,8(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 8);
	ctx.f5.f64 = double(temp.f32);
	// addi r11,r11,16
	ctx.r11.s64 = ctx.r11.s64 + 16;
	// lfs f6,8(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 8);
	ctx.f6.f64 = double(temp.f32);
	// lfs f3,12(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 12);
	ctx.f3.f64 = double(temp.f32);
	// fadds f23,f6,f5
	ctx.f23.f64 = double(float(ctx.f6.f64 + ctx.f5.f64));
	// lfs f4,0(r27)
	temp.u32 = REX_LOAD_U32(ctx.r27.u32 + 0);
	ctx.f4.f64 = double(temp.f32);
	// fsubs f6,f6,f5
	ctx.f6.f64 = double(float(ctx.f6.f64 - ctx.f5.f64));
	// lfs f1,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f1.f64 = double(temp.f32);
	// fadds f5,f3,f4
	ctx.f5.f64 = double(float(ctx.f3.f64 + ctx.f4.f64));
	// lfs f2,0(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 0);
	ctx.f2.f64 = double(temp.f32);
	// fsubs f4,f4,f3
	ctx.f4.f64 = double(float(ctx.f4.f64 - ctx.f3.f64));
	// lfs f30,0(r25)
	temp.u32 = REX_LOAD_U32(ctx.r25.u32 + 0);
	ctx.f30.f64 = double(temp.f32);
	// fadds f3,f1,f2
	ctx.f3.f64 = double(float(ctx.f1.f64 + ctx.f2.f64));
	// lfs f31,0(r26)
	temp.u32 = REX_LOAD_U32(ctx.r26.u32 + 0);
	ctx.f31.f64 = double(temp.f32);
	// fsubs f2,f2,f1
	ctx.f2.f64 = double(float(ctx.f2.f64 - ctx.f1.f64));
	// lfs f28,8(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 8);
	ctx.f28.f64 = double(temp.f32);
	// fadds f1,f30,f31
	ctx.f1.f64 = double(float(ctx.f30.f64 + ctx.f31.f64));
	// lfs f29,8(r6)
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 8);
	ctx.f29.f64 = double(temp.f32);
	// fsubs f31,f31,f30
	ctx.f31.f64 = double(float(ctx.f31.f64 - ctx.f30.f64));
	// fadds f30,f29,f28
	ctx.f30.f64 = double(float(ctx.f29.f64 + ctx.f28.f64));
	// lfs f26,0(r24)
	temp.u32 = REX_LOAD_U32(ctx.r24.u32 + 0);
	ctx.f26.f64 = double(temp.f32);
	// fsubs f29,f28,f29
	ctx.f29.f64 = double(float(ctx.f28.f64 - ctx.f29.f64));
	// lfs f27,12(r6)
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 12);
	ctx.f27.f64 = double(temp.f32);
	// lfs f25,0(r6)
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 0);
	ctx.f25.f64 = double(temp.f32);
	// fadds f28,f26,f27
	ctx.f28.f64 = double(float(ctx.f26.f64 + ctx.f27.f64));
	// lfs f24,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f24.f64 = double(temp.f32);
	// fsubs f27,f26,f27
	ctx.f27.f64 = double(float(ctx.f26.f64 - ctx.f27.f64));
	// fadds f26,f25,f24
	ctx.f26.f64 = double(float(ctx.f25.f64 + ctx.f24.f64));
	// fadds f22,f30,f23
	ctx.f22.f64 = double(float(ctx.f30.f64 + ctx.f23.f64));
	// fsubs f23,f23,f30
	ctx.f23.f64 = double(float(ctx.f23.f64 - ctx.f30.f64));
	// fadds f30,f29,f4
	ctx.f30.f64 = double(float(ctx.f29.f64 + ctx.f4.f64));
	// fadds f21,f28,f5
	ctx.f21.f64 = double(float(ctx.f28.f64 + ctx.f5.f64));
	// fsubs f20,f5,f28
	ctx.f20.f64 = double(float(ctx.f5.f64 - ctx.f28.f64));
	// fsubs f28,f24,f25
	ctx.f28.f64 = double(float(ctx.f24.f64 - ctx.f25.f64));
	// fadds f25,f26,f3
	ctx.f25.f64 = double(float(ctx.f26.f64 + ctx.f3.f64));
	// fsubs f24,f3,f26
	ctx.f24.f64 = double(float(ctx.f3.f64 - ctx.f26.f64));
	// lfs f3,0(r23)
	temp.u32 = REX_LOAD_U32(ctx.r23.u32 + 0);
	ctx.f3.f64 = double(temp.f32);
	// lfs f26,0(r22)
	temp.u32 = REX_LOAD_U32(ctx.r22.u32 + 0);
	ctx.f26.f64 = double(temp.f32);
	// fsubs f5,f6,f27
	ctx.f5.f64 = double(float(ctx.f6.f64 - ctx.f27.f64));
	// stfs f25,0(r5)
	temp.f32 = float(ctx.f25.f64);
	REX_STORE_U32(ctx.r5.u32 + 0, temp.u32);
	// stfs f22,8(r5)
	temp.f32 = float(ctx.f22.f64);
	REX_STORE_U32(ctx.r5.u32 + 8, temp.u32);
	// addi r5,r5,-16
	ctx.r5.s64 = ctx.r5.s64 + -16;
	// stfs f21,0(r27)
	temp.f32 = float(ctx.f21.f64);
	REX_STORE_U32(ctx.r27.u32 + 0, temp.u32);
	// fmuls f25,f10,f30
	ctx.f25.f64 = double(float(ctx.f10.f64 * ctx.f30.f64));
	// fmuls f22,f10,f5
	ctx.f22.f64 = double(float(ctx.f10.f64 * ctx.f5.f64));
	// fadds f10,f28,f31
	ctx.f10.f64 = double(float(ctx.f28.f64 + ctx.f31.f64));
	// fmsubs f25,f9,f5,f25
	ctx.f25.f64 = double(float(std::fma(ctx.f9.f64, ctx.f5.f64, -ctx.f25.f64)));
	// fmadds f30,f9,f30,f22
	ctx.f30.f64 = double(float(std::fma(ctx.f9.f64, ctx.f30.f64, ctx.f22.f64)));
	// fsubs f9,f26,f3
	ctx.f9.f64 = double(float(ctx.f26.f64 - ctx.f3.f64));
	// fadds f5,f3,f26
	ctx.f5.f64 = double(float(ctx.f3.f64 + ctx.f26.f64));
	// fmuls f3,f0,f10
	ctx.f3.f64 = double(float(ctx.f0.f64 * ctx.f10.f64));
	// fmr f26,f10
	ctx.f26.f64 = ctx.f10.f64;
	// fsubs f10,f4,f29
	ctx.f10.f64 = double(float(ctx.f4.f64 - ctx.f29.f64));
	// fadds f6,f27,f6
	ctx.f6.f64 = double(float(ctx.f27.f64 + ctx.f6.f64));
	// fsubs f4,f2,f9
	ctx.f4.f64 = double(float(ctx.f2.f64 - ctx.f9.f64));
	// fadds f29,f5,f1
	ctx.f29.f64 = double(float(ctx.f5.f64 + ctx.f1.f64));
	// stfs f29,0(r26)
	temp.f32 = float(ctx.f29.f64);
	REX_STORE_U32(ctx.r26.u32 + 0, temp.u32);
	// fsubs f5,f1,f5
	ctx.f5.f64 = double(float(ctx.f1.f64 - ctx.f5.f64));
	// stfs f5,0(r22)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r22.u32 + 0, temp.u32);
	// stfs f24,0(r31)
	temp.f32 = float(ctx.f24.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// fadds f9,f9,f2
	ctx.f9.f64 = double(float(ctx.f9.f64 + ctx.f2.f64));
	// fmr f5,f10
	ctx.f5.f64 = ctx.f10.f64;
	// stfs f23,8(r31)
	temp.f32 = float(ctx.f23.f64);
	REX_STORE_U32(ctx.r31.u32 + 8, temp.u32);
	// fmr f1,f10
	ctx.f1.f64 = ctx.f10.f64;
	// stfs f20,0(r24)
	temp.f32 = float(ctx.f20.f64);
	REX_STORE_U32(ctx.r24.u32 + 0, temp.u32);
	// fsubs f10,f31,f28
	ctx.f10.f64 = double(float(ctx.f31.f64 - ctx.f28.f64));
	// stfs f25,8(r7)
	temp.f32 = float(ctx.f25.f64);
	REX_STORE_U32(ctx.r7.u32 + 8, temp.u32);
	// stfs f30,12(r7)
	temp.f32 = float(ctx.f30.f64);
	REX_STORE_U32(ctx.r7.u32 + 12, temp.u32);
	// addi r31,r31,-16
	ctx.r31.s64 = ctx.r31.s64 + -16;
	// fmuls f31,f0,f4
	ctx.f31.f64 = double(float(ctx.f0.f64 * ctx.f4.f64));
	// fmsubs f4,f13,f4,f3
	ctx.f4.f64 = double(float(std::fma(ctx.f13.f64, ctx.f4.f64, -ctx.f3.f64)));
	// stfs f4,0(r7)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// addi r7,r7,-16
	ctx.r7.s64 = ctx.r7.s64 + -16;
	// fmadds f4,f13,f26,f31
	ctx.f4.f64 = double(float(std::fma(ctx.f13.f64, ctx.f26.f64, ctx.f31.f64)));
	// stfs f4,0(r25)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r25.u32 + 0, temp.u32);
	// fmuls f4,f7,f6
	ctx.f4.f64 = double(float(ctx.f7.f64 * ctx.f6.f64));
	// fmuls f6,f8,f6
	ctx.f6.f64 = double(float(ctx.f8.f64 * ctx.f6.f64));
	// fmadds f8,f8,f5,f4
	ctx.f8.f64 = double(float(std::fma(ctx.f8.f64, ctx.f5.f64, ctx.f4.f64)));
	// stfs f8,8(r6)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r6.u32 + 8, temp.u32);
	// fmsubs f8,f7,f1,f6
	ctx.f8.f64 = double(float(std::fma(ctx.f7.f64, ctx.f1.f64, -ctx.f6.f64)));
	// stfs f8,12(r6)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r6.u32 + 12, temp.u32);
	// fmuls f8,f11,f9
	ctx.f8.f64 = double(float(ctx.f11.f64 * ctx.f9.f64));
	// fmuls f9,f12,f9
	ctx.f9.f64 = double(float(ctx.f12.f64 * ctx.f9.f64));
	// fmadds f8,f12,f10,f8
	ctx.f8.f64 = double(float(std::fma(ctx.f12.f64, ctx.f10.f64, ctx.f8.f64)));
	// stfs f8,0(r6)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r6.u32 + 0, temp.u32);
	// fmsubs f10,f11,f10,f9
	ctx.f10.f64 = double(float(std::fma(ctx.f11.f64, ctx.f10.f64, -ctx.f9.f64)));
	// addi r6,r6,-16
	ctx.r6.s64 = ctx.r6.s64 + -16;
	// stfs f10,0(r23)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r23.u32 + 0, temp.u32);
	// bne cr6,0x8295aec0
	if (!ctx.cr6.eq) goto loc_8295AEC0;
loc_8295B1F0:
	// add r7,r3,r28
	ctx.r7.u64 = ctx.r3.u64 + ctx.r28.u64;
	// fadds f13,f13,f15
	ctx.fpscr.disableFlushMode();
	ctx.f13.f64 = double(float(ctx.f13.f64 + ctx.f15.f64));
	// fadds f10,f0,f15
	ctx.f10.f64 = double(float(ctx.f0.f64 + ctx.f15.f64));
	// rlwinm r11,r28,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r28.u32 | (ctx.r28.u64 << 32), 2) & 0xFFFFFFFC;
	// add r6,r7,r3
	ctx.r6.u64 = ctx.r7.u64 + ctx.r3.u64;
	// fsubs f12,f12,f15
	ctx.f12.f64 = double(float(ctx.f12.f64 - ctx.f15.f64));
	// addi r9,r7,-2
	ctx.r9.s64 = ctx.r7.s64 + -2;
	// fsubs f11,f11,f15
	ctx.f11.f64 = double(float(ctx.f11.f64 - ctx.f15.f64));
	// addi r8,r6,-2
	ctx.r8.s64 = ctx.r6.s64 + -2;
	// add r5,r6,r3
	ctx.r5.u64 = ctx.r6.u64 + ctx.r3.u64;
	// rlwinm r30,r9,2,0,29
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r6,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r31,r8,2,0,29
	ctx.r31.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r29,r21,2,0,29
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r21.u32 | (ctx.r21.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r7,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// fmuls f0,f13,f17
	ctx.f0.f64 = double(float(ctx.f13.f64 * ctx.f17.f64));
	// rlwinm r8,r5,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 2) & 0xFFFFFFFC;
	// fmuls f13,f10,f17
	ctx.f13.f64 = double(float(ctx.f10.f64 * ctx.f17.f64));
	// addi r3,r5,-2
	ctx.r3.s64 = ctx.r5.s64 + -2;
	// lfsx f8,r30,r4
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + ctx.r4.u32);
	ctx.f8.f64 = double(temp.f32);
	// add r11,r11,r4
	ctx.r11.u64 = ctx.r11.u64 + ctx.r4.u64;
	// lfsx f6,r31,r4
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + ctx.r4.u32);
	ctx.f6.f64 = double(temp.f32);
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// lfsx f10,r29,r4
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + ctx.r4.u32);
	ctx.f10.f64 = double(temp.f32);
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// fadds f3,f10,f6
	ctx.f3.f64 = double(float(ctx.f10.f64 + ctx.f6.f64));
	// add r8,r8,r4
	ctx.r8.u64 = ctx.r8.u64 + ctx.r4.u64;
	// fsubs f10,f10,f6
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f6.f64));
	// rlwinm r3,r3,2,0,29
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 2) & 0xFFFFFFFC;
	// fmuls f12,f12,f16
	ctx.f12.f64 = double(float(ctx.f12.f64 * ctx.f16.f64));
	// lfs f9,-4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -4);
	ctx.f9.f64 = double(temp.f32);
	// fmuls f11,f11,f16
	ctx.f11.f64 = double(float(ctx.f11.f64 * ctx.f16.f64));
	// lfs f5,-4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -4);
	ctx.f5.f64 = double(temp.f32);
	// addi r26,r5,2
	ctx.r26.s64 = ctx.r5.s64 + 2;
	// lfs f7,-4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -4);
	ctx.f7.f64 = double(temp.f32);
	// fadds f2,f9,f5
	ctx.f2.f64 = double(float(ctx.f9.f64 + ctx.f5.f64));
	// lfs f6,-4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + -4);
	ctx.f6.f64 = double(temp.f32);
	// fsubs f9,f9,f5
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f5.f64));
	// lfsx f4,r3,r4
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + ctx.r4.u32);
	ctx.f4.f64 = double(temp.f32);
	// fadds f5,f7,f6
	ctx.f5.f64 = double(float(ctx.f7.f64 + ctx.f6.f64));
	// fadds f1,f8,f4
	ctx.f1.f64 = double(float(ctx.f8.f64 + ctx.f4.f64));
	// addi r25,r7,2
	ctx.r25.s64 = ctx.r7.s64 + 2;
	// fsubs f8,f8,f4
	ctx.f8.f64 = double(float(ctx.f8.f64 - ctx.f4.f64));
	// fsubs f7,f7,f6
	ctx.f7.f64 = double(float(ctx.f7.f64 - ctx.f6.f64));
	// fadds f4,f5,f2
	ctx.f4.f64 = double(float(ctx.f5.f64 + ctx.f2.f64));
	// stfs f4,-4(r11)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r11.u32 + -4, temp.u32);
	// fadds f6,f1,f3
	ctx.f6.f64 = double(float(ctx.f1.f64 + ctx.f3.f64));
	// stfsx f6,r29,r4
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r29.u32 + ctx.r4.u32, temp.u32);
	// fsubs f5,f2,f5
	ctx.f5.f64 = double(float(ctx.f2.f64 - ctx.f5.f64));
	// stfs f5,-4(r10)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r10.u32 + -4, temp.u32);
	// fadds f5,f8,f9
	ctx.f5.f64 = double(float(ctx.f8.f64 + ctx.f9.f64));
	// fsubs f6,f3,f1
	ctx.f6.f64 = double(float(ctx.f3.f64 - ctx.f1.f64));
	// stfsx f6,r30,r4
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r30.u32 + ctx.r4.u32, temp.u32);
	// fsubs f6,f10,f7
	ctx.f6.f64 = double(float(ctx.f10.f64 - ctx.f7.f64));
	// addi r30,r28,3
	ctx.r30.s64 = ctx.r28.s64 + 3;
	// fadds f10,f7,f10
	ctx.f10.f64 = double(float(ctx.f7.f64 + ctx.f10.f64));
	// fsubs f9,f9,f8
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f8.f64));
	// rlwinm r29,r30,2,0,29
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// fmuls f4,f0,f5
	ctx.f4.f64 = double(float(ctx.f0.f64 * ctx.f5.f64));
	// fmuls f3,f0,f6
	ctx.f3.f64 = double(float(ctx.f0.f64 * ctx.f6.f64));
	// fmsubs f8,f13,f6,f4
	ctx.f8.f64 = double(float(std::fma(ctx.f13.f64, ctx.f6.f64, -ctx.f4.f64)));
	// stfsx f8,r31,r4
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r31.u32 + ctx.r4.u32, temp.u32);
	// addi r31,r6,2
	ctx.r31.s64 = ctx.r6.s64 + 2;
	// fneg f6,f15
	ctx.f6.u64 = ctx.f15.u64 ^ 0x8000000000000000;
	// fmadds f8,f13,f5,f3
	ctx.f8.f64 = double(float(std::fma(ctx.f13.f64, ctx.f5.f64, ctx.f3.f64)));
	// stfs f8,-4(r9)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r9.u32 + -4, temp.u32);
	// fmuls f8,f12,f10
	ctx.f8.f64 = double(float(ctx.f12.f64 * ctx.f10.f64));
	// addi r6,r6,3
	ctx.r6.s64 = ctx.r6.s64 + 3;
	// fmuls f10,f11,f10
	ctx.f10.f64 = double(float(ctx.f11.f64 * ctx.f10.f64));
	// rlwinm r30,r6,2,0,29
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// fmadds f8,f11,f9,f8
	ctx.f8.f64 = double(float(std::fma(ctx.f11.f64, ctx.f9.f64, ctx.f8.f64)));
	// stfsx f8,r3,r4
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r3.u32 + ctx.r4.u32, temp.u32);
	// addi r3,r28,2
	ctx.r3.s64 = ctx.r28.s64 + 2;
	// fmsubs f10,f12,f9,f10
	ctx.f10.f64 = double(float(std::fma(ctx.f12.f64, ctx.f9.f64, -ctx.f10.f64)));
	// stfs f10,-4(r8)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r8.u32 + -4, temp.u32);
	// rlwinm r28,r31,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f10,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f10.f64 = double(temp.f32);
	// rlwinm r27,r3,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f9,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f9.f64 = double(temp.f32);
	// lfs f8,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f8.f64 = double(temp.f32);
	// lfs f7,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f7.f64 = double(temp.f32);
	// fadds f2,f10,f9
	ctx.f2.f64 = double(float(ctx.f10.f64 + ctx.f9.f64));
	// lfs f4,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f4.f64 = double(temp.f32);
	// fsubs f10,f10,f9
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f9.f64));
	// lfs f5,4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4);
	ctx.f5.f64 = double(temp.f32);
	// fadds f9,f7,f8
	ctx.f9.f64 = double(float(ctx.f7.f64 + ctx.f8.f64));
	// lfs f3,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f3.f64 = double(temp.f32);
	// fsubs f8,f7,f8
	ctx.f8.f64 = double(float(ctx.f7.f64 - ctx.f8.f64));
	// lfs f1,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f1.f64 = double(temp.f32);
	// fadds f7,f5,f4
	ctx.f7.f64 = double(float(ctx.f5.f64 + ctx.f4.f64));
	// rlwinm r31,r26,2,0,29
	ctx.r31.u64 = __builtin_rotateleft64(ctx.r26.u32 | (ctx.r26.u64 << 32), 2) & 0xFFFFFFFC;
	// fsubs f5,f5,f4
	ctx.f5.f64 = double(float(ctx.f5.f64 - ctx.f4.f64));
	// rlwinm r3,r25,2,0,29
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r25.u32 | (ctx.r25.u64 << 32), 2) & 0xFFFFFFFC;
	// fadds f4,f1,f3
	ctx.f4.f64 = double(float(ctx.f1.f64 + ctx.f3.f64));
	// addi r6,r5,3
	ctx.r6.s64 = ctx.r5.s64 + 3;
	// fsubs f3,f1,f3
	ctx.f3.f64 = double(float(ctx.f1.f64 - ctx.f3.f64));
	// addi r7,r7,3
	ctx.r7.s64 = ctx.r7.s64 + 3;
	// rlwinm r6,r6,2,0,29
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r7,r7,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// fadds f1,f9,f2
	ctx.f1.f64 = double(float(ctx.f9.f64 + ctx.f2.f64));
	// stfs f1,0(r11)
	temp.f32 = float(ctx.f1.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// fsubs f9,f2,f9
	ctx.f9.f64 = double(float(ctx.f2.f64 - ctx.f9.f64));
	// fadds f2,f4,f7
	ctx.f2.f64 = double(float(ctx.f4.f64 + ctx.f7.f64));
	// stfs f2,4(r11)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r11.u32 + 4, temp.u32);
	// fsubs f7,f7,f4
	ctx.f7.f64 = double(float(ctx.f7.f64 - ctx.f4.f64));
	// stfs f9,0(r10)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// stfs f7,4(r10)
	temp.f32 = float(ctx.f7.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// fsubs f9,f10,f3
	ctx.f9.f64 = double(float(ctx.f10.f64 - ctx.f3.f64));
	// fadds f7,f8,f5
	ctx.f7.f64 = double(float(ctx.f8.f64 + ctx.f5.f64));
	// fadds f10,f3,f10
	ctx.f10.f64 = double(float(ctx.f3.f64 + ctx.f10.f64));
	// fsubs f4,f9,f7
	ctx.f4.f64 = double(float(ctx.f9.f64 - ctx.f7.f64));
	// fadds f7,f7,f9
	ctx.f7.f64 = double(float(ctx.f7.f64 + ctx.f9.f64));
	// fsubs f9,f5,f8
	ctx.f9.f64 = double(float(ctx.f5.f64 - ctx.f8.f64));
	// fmuls f8,f4,f15
	ctx.f8.f64 = double(float(ctx.f4.f64 * ctx.f15.f64));
	// stfs f8,0(r9)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// fmuls f8,f7,f15
	ctx.f8.f64 = double(float(ctx.f7.f64 * ctx.f15.f64));
	// stfs f8,4(r9)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// fadds f7,f9,f10
	ctx.f7.f64 = double(float(ctx.f9.f64 + ctx.f10.f64));
	// lfsx f8,r31,r4
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + ctx.r4.u32);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f10,f9,f10
	ctx.f10.f64 = double(float(ctx.f9.f64 - ctx.f10.f64));
	// lfsx f3,r6,r4
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + ctx.r4.u32);
	ctx.f3.f64 = double(temp.f32);
	// fmuls f9,f7,f6
	ctx.f9.f64 = double(float(ctx.f7.f64 * ctx.f6.f64));
	// stfs f9,0(r8)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// fmuls f10,f10,f6
	ctx.f10.f64 = double(float(ctx.f10.f64 * ctx.f6.f64));
	// stfs f10,4(r8)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// lfsx f9,r28,r4
	temp.u32 = REX_LOAD_U32(ctx.r28.u32 + ctx.r4.u32);
	ctx.f9.f64 = double(temp.f32);
	// lfsx f10,r27,r4
	temp.u32 = REX_LOAD_U32(ctx.r27.u32 + ctx.r4.u32);
	ctx.f10.f64 = double(temp.f32);
	// lfsx f6,r30,r4
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + ctx.r4.u32);
	ctx.f6.f64 = double(temp.f32);
	// fadds f4,f10,f9
	ctx.f4.f64 = double(float(ctx.f10.f64 + ctx.f9.f64));
	// lfsx f7,r29,r4
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + ctx.r4.u32);
	ctx.f7.f64 = double(temp.f32);
	// fsubs f10,f10,f9
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f9.f64));
	// lfsx f5,r3,r4
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + ctx.r4.u32);
	ctx.f5.f64 = double(temp.f32);
	// fadds f9,f7,f6
	ctx.f9.f64 = double(float(ctx.f7.f64 + ctx.f6.f64));
	// fsubs f7,f7,f6
	ctx.f7.f64 = double(float(ctx.f7.f64 - ctx.f6.f64));
	// fadds f6,f5,f8
	ctx.f6.f64 = double(float(ctx.f5.f64 + ctx.f8.f64));
	// fsubs f8,f5,f8
	ctx.f8.f64 = double(float(ctx.f5.f64 - ctx.f8.f64));
	// lfsx f5,r7,r4
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + ctx.r4.u32);
	ctx.f5.f64 = double(temp.f32);
	// fadds f2,f6,f4
	ctx.f2.f64 = double(float(ctx.f6.f64 + ctx.f4.f64));
	// stfsx f2,r27,r4
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r27.u32 + ctx.r4.u32, temp.u32);
	// fsubs f4,f4,f6
	ctx.f4.f64 = double(float(ctx.f4.f64 - ctx.f6.f64));
	// fadds f6,f5,f3
	ctx.f6.f64 = double(float(ctx.f5.f64 + ctx.f3.f64));
	// fsubs f5,f5,f3
	ctx.f5.f64 = double(float(ctx.f5.f64 - ctx.f3.f64));
	// fadds f3,f6,f9
	ctx.f3.f64 = double(float(ctx.f6.f64 + ctx.f9.f64));
	// stfsx f3,r29,r4
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r29.u32 + ctx.r4.u32, temp.u32);
	// fsubs f6,f9,f6
	ctx.f6.f64 = double(float(ctx.f9.f64 - ctx.f6.f64));
	// stfsx f6,r7,r4
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r7.u32 + ctx.r4.u32, temp.u32);
	// fadds f6,f8,f7
	ctx.f6.f64 = double(float(ctx.f8.f64 + ctx.f7.f64));
	// stfsx f4,r3,r4
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r3.u32 + ctx.r4.u32, temp.u32);
	// fsubs f9,f10,f5
	ctx.f9.f64 = double(float(ctx.f10.f64 - ctx.f5.f64));
	// fmuls f4,f13,f6
	ctx.f4.f64 = double(float(ctx.f13.f64 * ctx.f6.f64));
	// fmuls f3,f13,f9
	ctx.f3.f64 = double(float(ctx.f13.f64 * ctx.f9.f64));
	// fadds f13,f5,f10
	ctx.f13.f64 = double(float(ctx.f5.f64 + ctx.f10.f64));
	// fsubs f10,f7,f8
	ctx.f10.f64 = double(float(ctx.f7.f64 - ctx.f8.f64));
	// fmsubs f9,f0,f9,f4
	ctx.f9.f64 = double(float(std::fma(ctx.f0.f64, ctx.f9.f64, -ctx.f4.f64)));
	// stfsx f9,r28,r4
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r28.u32 + ctx.r4.u32, temp.u32);
	// fmadds f0,f0,f6,f3
	ctx.f0.f64 = double(float(std::fma(ctx.f0.f64, ctx.f6.f64, ctx.f3.f64)));
	// stfsx f0,r30,r4
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r30.u32 + ctx.r4.u32, temp.u32);
	// fmuls f0,f11,f13
	ctx.f0.f64 = double(float(ctx.f11.f64 * ctx.f13.f64));
	// fmuls f13,f12,f13
	ctx.f13.f64 = double(float(ctx.f12.f64 * ctx.f13.f64));
	// fmadds f0,f12,f10,f0
	ctx.f0.f64 = double(float(std::fma(ctx.f12.f64, ctx.f10.f64, ctx.f0.f64)));
	// stfsx f0,r31,r4
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + ctx.r4.u32, temp.u32);
	// fmsubs f0,f11,f10,f13
	ctx.f0.f64 = double(float(std::fma(ctx.f11.f64, ctx.f10.f64, -ctx.f13.f64)));
	// stfsx f0,r6,r4
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r6.u32 + ctx.r4.u32, temp.u32);
	// addi r12,r1,-96
	ctx.r12.s64 = ctx.r1.s64 + -96;
	// bl 0x824144dc
	ctx.lr = 0x8295B484;
	__restfpr_14(ctx, base);
	// b 0x824131dc
	__restgprlr_21(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295B488) {
	REX_FUNC_PROLOGUE(sub_8295B488);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x8241319c
	ctx.lr = 0x8295B490;
	__savegprlr_25(ctx, base);
	// addi r12,r1,-64
	ctx.r12.s64 = ctx.r1.s64 + -64;
	// bl 0x82414490
	ctx.lr = 0x8295B498;
	__savefpr_14(ctx, base);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f9,4(r4)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 4);
	ctx.f9.f64 = double(temp.f32);
	// srawi r29,r3,3
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0x7) != 0);
	ctx.r29.s64 = ctx.r3.s32 >> 3;
	// fneg f1,f9
	ctx.f1.u64 = ctx.f9.u64 ^ 0x8000000000000000;
	// lfs f10,0(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 0);
	ctx.f10.f64 = double(temp.f32);
	// addi r30,r5,8
	ctx.r30.s64 = ctx.r5.s64 + 8;
	// addi r26,r29,-2
	ctx.r26.s64 = ctx.r29.s64 + -2;
	// lfs f0,5288(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 5288);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// fmr f12,f0
	ctx.f12.f64 = ctx.f0.f64;
	// cmpwi cr6,r26,2
	ctx.cr6.compare<int32_t>(ctx.r26.s32, 2, ctx.xer);
	// lfs f13,4712(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4712);
	ctx.f13.f64 = double(temp.f32);
	// rlwinm r11,r29,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 1) & 0xFFFFFFFE;
	// fmr f11,f13
	ctx.f11.f64 = ctx.f13.f64;
	// rlwinm r9,r11,1,0,30
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// rlwinm r10,r11,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r8,r9,r11
	ctx.r8.u64 = ctx.r9.u64 + ctx.r11.u64;
	// rlwinm r9,r9,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r8,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// add r8,r8,r4
	ctx.r8.u64 = ctx.r8.u64 + ctx.r4.u64;
	// lfs f7,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f7.f64 = double(temp.f32);
	// lfs f8,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f8.f64 = double(temp.f32);
	// fadds f3,f10,f7
	ctx.f3.f64 = double(float(ctx.f10.f64 + ctx.f7.f64));
	// lfs f5,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f5.f64 = double(temp.f32);
	// fsubs f10,f10,f7
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f7.f64));
	// lfs f4,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f4.f64 = double(temp.f32);
	// fadds f2,f8,f5
	ctx.f2.f64 = double(float(ctx.f8.f64 + ctx.f5.f64));
	// lfs f6,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f6.f64 = double(temp.f32);
	// fsubs f8,f8,f5
	ctx.f8.f64 = double(float(ctx.f8.f64 - ctx.f5.f64));
	// lfs f7,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f7.f64 = double(temp.f32);
	// fsubs f9,f4,f9
	ctx.f9.f64 = double(float(ctx.f4.f64 - ctx.f9.f64));
	// fadds f5,f6,f7
	ctx.f5.f64 = double(float(ctx.f6.f64 + ctx.f7.f64));
	// fsubs f1,f1,f4
	ctx.f1.f64 = double(float(ctx.f1.f64 - ctx.f4.f64));
	// fsubs f7,f6,f7
	ctx.f7.f64 = double(float(ctx.f6.f64 - ctx.f7.f64));
	// fadds f6,f2,f3
	ctx.f6.f64 = double(float(ctx.f2.f64 + ctx.f3.f64));
	// stfs f6,0(r4)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r4.u32 + 0, temp.u32);
	// fsubs f6,f3,f2
	ctx.f6.f64 = double(float(ctx.f3.f64 - ctx.f2.f64));
	// fadds f4,f8,f9
	ctx.f4.f64 = double(float(ctx.f8.f64 + ctx.f9.f64));
	// fsubs f9,f9,f8
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f8.f64));
	// fsubs f8,f1,f5
	ctx.f8.f64 = double(float(ctx.f1.f64 - ctx.f5.f64));
	// stfs f8,4(r4)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r4.u32 + 4, temp.u32);
	// fadds f8,f5,f1
	ctx.f8.f64 = double(float(ctx.f5.f64 + ctx.f1.f64));
	// stfs f8,4(r10)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// stfs f6,0(r10)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// fadds f8,f7,f10
	ctx.f8.f64 = double(float(ctx.f7.f64 + ctx.f10.f64));
	// stfs f8,0(r9)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// fsubs f10,f10,f7
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f7.f64));
	// stfs f4,4(r9)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// stfs f10,0(r8)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f9,4(r8)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// lfs f15,4(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 4);
	ctx.f15.f64 = double(temp.f32);
	// lfs f17,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f17.f64 = double(temp.f32);
	// lfs f16,12(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 12);
	ctx.f16.f64 = double(temp.f32);
	// ble cr6,0x8295b90c
	if (!ctx.cr6.gt) goto loc_8295B90C;
	// addi r9,r11,4
	ctx.r9.s64 = ctx.r11.s64 + 4;
	// rlwinm r10,r11,4,0,27
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 4) & 0xFFFFFFF0;
	// addi r6,r11,2
	ctx.r6.s64 = ctx.r11.s64 + 2;
	// addi r31,r26,-3
	ctx.r31.s64 = ctx.r26.s64 + -3;
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// rlwinm r8,r9,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r6,3,0,28
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 3) & 0xFFFFFFF8;
	// rlwinm r6,r31,30,2,31
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 30) & 0x3FFFFFFF;
	// addi r31,r10,-16
	ctx.r31.s64 = ctx.r10.s64 + -16;
	// rlwinm r10,r11,1,0,30
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// addi r7,r11,-4
	ctx.r7.s64 = ctx.r11.s64 + -4;
	// add r10,r11,r10
	ctx.r10.u64 = ctx.r11.u64 + ctx.r10.u64;
	// addi r5,r11,-2
	ctx.r5.s64 = ctx.r11.s64 + -2;
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r28,r6,1
	ctx.r28.s64 = ctx.r6.s64 + 1;
	// add r6,r10,r4
	ctx.r6.u64 = ctx.r10.u64 + ctx.r4.u64;
	// rlwinm r3,r7,2,0,29
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r5,r5,3,0,28
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 3) & 0xFFFFFFF8;
	// addi r10,r6,16
	ctx.r10.s64 = ctx.r6.s64 + 16;
	// addi r7,r4,16
	ctx.r7.s64 = ctx.r4.s64 + 16;
	// add r8,r8,r4
	ctx.r8.u64 = ctx.r8.u64 + ctx.r4.u64;
	// add r3,r3,r4
	ctx.r3.u64 = ctx.r3.u64 + ctx.r4.u64;
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// add r5,r5,r4
	ctx.r5.u64 = ctx.r5.u64 + ctx.r4.u64;
	// addi r6,r6,-16
	ctx.r6.s64 = ctx.r6.s64 + -16;
loc_8295B5DC:
	// lfs f9,0(r10)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f9.f64 = double(temp.f32);
	// addi r30,r30,16
	ctx.r30.s64 = ctx.r30.s64 + 16;
	// lfs f10,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f10.f64 = double(temp.f32);
	// lfs f8,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f8.f64 = double(temp.f32);
	// lfs f7,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f7.f64 = double(temp.f32);
	// fadds f3,f8,f10
	ctx.f3.f64 = double(float(ctx.f8.f64 + ctx.f10.f64));
	// fadds f1,f7,f9
	ctx.f1.f64 = double(float(ctx.f7.f64 + ctx.f9.f64));
	// lfs f4,-8(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + -8);
	ctx.f4.f64 = double(temp.f32);
	// fsubs f29,f7,f9
	ctx.f29.f64 = double(float(ctx.f7.f64 - ctx.f9.f64));
	// lfs f9,-8(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -8);
	ctx.f9.f64 = double(temp.f32);
	// lfs f25,-8(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + -8);
	ctx.f25.f64 = double(temp.f32);
	// fsubs f30,f8,f10
	ctx.f30.f64 = double(float(ctx.f8.f64 - ctx.f10.f64));
	// lfs f22,4(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 4);
	ctx.f22.f64 = double(temp.f32);
	// lfs f6,-4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + -4);
	ctx.f6.f64 = double(temp.f32);
	// fneg f21,f6
	ctx.f21.u64 = ctx.f6.u64 ^ 0x8000000000000000;
	// lfs f5,4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 4);
	ctx.f5.f64 = double(temp.f32);
	// lfs f28,-8(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -8);
	ctx.f28.f64 = double(temp.f32);
	// fneg f20,f5
	ctx.f20.u64 = ctx.f5.u64 ^ 0x8000000000000000;
	// lfs f27,-4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -4);
	ctx.f27.f64 = double(temp.f32);
	// lfs f26,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f26.f64 = double(temp.f32);
	// lfs f24,-4(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + -4);
	ctx.f24.f64 = double(temp.f32);
	// fadds f19,f1,f3
	ctx.f19.f64 = double(float(ctx.f1.f64 + ctx.f3.f64));
	// lfs f8,-4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -4);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f14,f3,f1
	ctx.f14.f64 = double(float(ctx.f3.f64 - ctx.f1.f64));
	// lfs f23,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f23.f64 = double(temp.f32);
	// fadds f3,f9,f4
	ctx.f3.f64 = double(float(ctx.f9.f64 + ctx.f4.f64));
	// lfs f7,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f7.f64 = double(temp.f32);
	// fsubs f4,f4,f9
	ctx.f4.f64 = double(float(ctx.f4.f64 - ctx.f9.f64));
	// lfs f10,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f10.f64 = double(temp.f32);
	// fadds f9,f0,f25
	ctx.f9.f64 = double(float(ctx.f0.f64 + ctx.f25.f64));
	// lfs f2,-8(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + -8);
	ctx.f2.f64 = double(temp.f32);
	// fsubs f0,f11,f22
	ctx.f0.f64 = double(float(ctx.f11.f64 - ctx.f22.f64));
	// stfs f0,-224(r1)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r1.u32 + -224, temp.u32);
	// lfs f31,-4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + -4);
	ctx.f31.f64 = double(temp.f32);
	// fsubs f21,f21,f8
	ctx.f21.f64 = double(float(ctx.f21.f64 - ctx.f8.f64));
	// stfs f19,0(r7)
	temp.f32 = float(ctx.f19.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// fadds f19,f27,f31
	ctx.f19.f64 = double(float(ctx.f27.f64 + ctx.f31.f64));
	// fsubs f6,f8,f6
	ctx.f6.f64 = double(float(ctx.f8.f64 - ctx.f6.f64));
	// fsubs f31,f31,f27
	ctx.f31.f64 = double(float(ctx.f31.f64 - ctx.f27.f64));
	// fadds f8,f13,f24
	ctx.f8.f64 = double(float(ctx.f13.f64 + ctx.f24.f64));
	// fadds f27,f23,f12
	ctx.f27.f64 = double(float(ctx.f23.f64 + ctx.f12.f64));
	// fadds f1,f28,f2
	ctx.f1.f64 = double(float(ctx.f28.f64 + ctx.f2.f64));
	// fsubs f2,f2,f28
	ctx.f2.f64 = double(float(ctx.f2.f64 - ctx.f28.f64));
	// fadds f18,f26,f10
	ctx.f18.f64 = double(float(ctx.f26.f64 + ctx.f10.f64));
	// fsubs f20,f20,f7
	ctx.f20.f64 = double(float(ctx.f20.f64 - ctx.f7.f64));
	// fsubs f28,f10,f26
	ctx.f28.f64 = double(float(ctx.f10.f64 - ctx.f26.f64));
	// fmuls f10,f9,f17
	ctx.f10.f64 = double(float(ctx.f9.f64 * ctx.f17.f64));
	// fsubs f5,f7,f5
	ctx.f5.f64 = double(float(ctx.f7.f64 - ctx.f5.f64));
	// fmr f0,f25
	ctx.f0.f64 = ctx.f25.f64;
	// fmuls f9,f8,f17
	ctx.f9.f64 = double(float(ctx.f8.f64 * ctx.f17.f64));
	// fmuls f8,f27,f16
	ctx.f8.f64 = double(float(ctx.f27.f64 * ctx.f16.f64));
	// fmr f13,f24
	ctx.f13.f64 = ctx.f24.f64;
	// fmr f12,f23
	ctx.f12.f64 = ctx.f23.f64;
	// fneg f11,f22
	ctx.f11.u64 = ctx.f22.u64 ^ 0x8000000000000000;
	// lfs f27,-224(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -224);
	ctx.f27.f64 = double(temp.f32);
	// fmuls f7,f27,f16
	ctx.f7.f64 = double(float(ctx.f27.f64 * ctx.f16.f64));
	// fadds f27,f1,f3
	ctx.f27.f64 = double(float(ctx.f1.f64 + ctx.f3.f64));
	// stfs f27,-8(r7)
	temp.f32 = float(ctx.f27.f64);
	REX_STORE_U32(ctx.r7.u32 + -8, temp.u32);
	// fsubs f3,f3,f1
	ctx.f3.f64 = double(float(ctx.f3.f64 - ctx.f1.f64));
	// fsubs f1,f21,f19
	ctx.f1.f64 = double(float(ctx.f21.f64 - ctx.f19.f64));
	// stfs f1,-4(r7)
	temp.f32 = float(ctx.f1.f64);
	REX_STORE_U32(ctx.r7.u32 + -4, temp.u32);
	// fsubs f1,f20,f18
	ctx.f1.f64 = double(float(ctx.f20.f64 - ctx.f18.f64));
	// stfs f1,4(r7)
	temp.f32 = float(ctx.f1.f64);
	REX_STORE_U32(ctx.r7.u32 + 4, temp.u32);
	// fadds f1,f19,f21
	ctx.f1.f64 = double(float(ctx.f19.f64 + ctx.f21.f64));
	// stfs f3,-8(r8)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r8.u32 + -8, temp.u32);
	// stfs f1,-4(r8)
	temp.f32 = float(ctx.f1.f64);
	REX_STORE_U32(ctx.r8.u32 + -4, temp.u32);
	// fadds f3,f31,f4
	ctx.f3.f64 = double(float(ctx.f31.f64 + ctx.f4.f64));
	// fadds f1,f2,f6
	ctx.f1.f64 = double(float(ctx.f2.f64 + ctx.f6.f64));
	// stfs f14,0(r8)
	temp.f32 = float(ctx.f14.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// fadds f27,f18,f20
	ctx.f27.f64 = double(float(ctx.f18.f64 + ctx.f20.f64));
	// stfs f27,4(r8)
	temp.f32 = float(ctx.f27.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// fmr f27,f3
	ctx.f27.f64 = ctx.f3.f64;
	// fmuls f26,f9,f1
	ctx.f26.f64 = double(float(ctx.f9.f64 * ctx.f1.f64));
	// fmr f25,f1
	ctx.f25.f64 = ctx.f1.f64;
	// fmuls f24,f9,f3
	ctx.f24.f64 = double(float(ctx.f9.f64 * ctx.f3.f64));
	// fadds f3,f28,f30
	ctx.f3.f64 = double(float(ctx.f28.f64 + ctx.f30.f64));
	// fadds f1,f29,f5
	ctx.f1.f64 = double(float(ctx.f29.f64 + ctx.f5.f64));
	// fmsubs f27,f10,f27,f26
	ctx.f27.f64 = double(float(std::fma(ctx.f10.f64, ctx.f27.f64, -ctx.f26.f64)));
	// stfs f27,-8(r9)
	temp.f32 = float(ctx.f27.f64);
	REX_STORE_U32(ctx.r9.u32 + -8, temp.u32);
	// fmadds f27,f10,f25,f24
	ctx.f27.f64 = double(float(std::fma(ctx.f10.f64, ctx.f25.f64, ctx.f24.f64)));
	// stfs f27,-4(r9)
	temp.f32 = float(ctx.f27.f64);
	REX_STORE_U32(ctx.r9.u32 + -4, temp.u32);
	// fmuls f27,f13,f1
	ctx.f27.f64 = double(float(ctx.f13.f64 * ctx.f1.f64));
	// addi r27,r31,4
	ctx.r27.s64 = ctx.r31.s64 + 4;
	// fsubs f4,f4,f31
	ctx.f4.f64 = double(float(ctx.f4.f64 - ctx.f31.f64));
	// addi r8,r8,16
	ctx.r8.s64 = ctx.r8.s64 + 16;
	// fmuls f26,f13,f3
	ctx.f26.f64 = double(float(ctx.f13.f64 * ctx.f3.f64));
	// addi r7,r7,16
	ctx.r7.s64 = ctx.r7.s64 + 16;
	// fsubs f6,f6,f2
	ctx.f6.f64 = double(float(ctx.f6.f64 - ctx.f2.f64));
	// fsubs f5,f5,f29
	ctx.f5.f64 = double(float(ctx.f5.f64 - ctx.f29.f64));
	// fmsubs f3,f0,f3,f27
	ctx.f3.f64 = double(float(std::fma(ctx.f0.f64, ctx.f3.f64, -ctx.f27.f64)));
	// stfs f3,0(r9)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// fmuls f2,f8,f4
	ctx.f2.f64 = double(float(ctx.f8.f64 * ctx.f4.f64));
	// fmadds f3,f0,f1,f26
	ctx.f3.f64 = double(float(std::fma(ctx.f0.f64, ctx.f1.f64, ctx.f26.f64)));
	// stfs f3,4(r9)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// fmr f1,f6
	ctx.f1.f64 = ctx.f6.f64;
	// addi r9,r9,16
	ctx.r9.s64 = ctx.r9.s64 + 16;
	// fmuls f4,f7,f4
	ctx.f4.f64 = double(float(ctx.f7.f64 * ctx.f4.f64));
	// fmr f3,f6
	ctx.f3.f64 = ctx.f6.f64;
	// fsubs f6,f30,f28
	ctx.f6.f64 = double(float(ctx.f30.f64 - ctx.f28.f64));
	// fmsubs f4,f8,f1,f4
	ctx.f4.f64 = double(float(std::fma(ctx.f8.f64, ctx.f1.f64, -ctx.f4.f64)));
	// stfs f4,-4(r10)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r10.u32 + -4, temp.u32);
	// fmadds f3,f7,f3,f2
	ctx.f3.f64 = double(float(std::fma(ctx.f7.f64, ctx.f3.f64, ctx.f2.f64)));
	// stfs f3,-8(r10)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r10.u32 + -8, temp.u32);
	// fmuls f4,f12,f6
	ctx.f4.f64 = double(float(ctx.f12.f64 * ctx.f6.f64));
	// fmuls f6,f11,f6
	ctx.f6.f64 = double(float(ctx.f11.f64 * ctx.f6.f64));
	// fmadds f4,f11,f5,f4
	ctx.f4.f64 = double(float(std::fma(ctx.f11.f64, ctx.f5.f64, ctx.f4.f64)));
	// stfs f4,0(r10)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// fmsubs f6,f12,f5,f6
	ctx.f6.f64 = double(float(std::fma(ctx.f12.f64, ctx.f5.f64, -ctx.f6.f64)));
	// stfs f6,4(r10)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// lfs f6,12(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 12);
	ctx.f6.f64 = double(temp.f32);
	// addi r10,r10,16
	ctx.r10.s64 = ctx.r10.s64 + 16;
	// fneg f24,f6
	ctx.f24.u64 = ctx.f6.u64 ^ 0x8000000000000000;
	// lfs f3,8(r6)
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 8);
	ctx.f3.f64 = double(temp.f32);
	// lfs f4,8(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 8);
	ctx.f4.f64 = double(temp.f32);
	// lfs f31,0(r6)
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 0);
	ctx.f31.f64 = double(temp.f32);
	// fadds f25,f3,f4
	ctx.f25.f64 = double(float(ctx.f3.f64 + ctx.f4.f64));
	// lfs f2,12(r6)
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 12);
	ctx.f2.f64 = double(temp.f32);
	// fsubs f4,f4,f3
	ctx.f4.f64 = double(float(ctx.f4.f64 - ctx.f3.f64));
	// lfs f1,0(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 0);
	ctx.f1.f64 = double(temp.f32);
	// fsubs f6,f2,f6
	ctx.f6.f64 = double(float(ctx.f2.f64 - ctx.f6.f64));
	// lfs f28,8(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 8);
	ctx.f28.f64 = double(temp.f32);
	// fadds f3,f31,f1
	ctx.f3.f64 = double(float(ctx.f31.f64 + ctx.f1.f64));
	// lfs f27,12(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 12);
	ctx.f27.f64 = double(temp.f32);
	// fsubs f1,f1,f31
	ctx.f1.f64 = double(float(ctx.f1.f64 - ctx.f31.f64));
	// lfs f26,12(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 12);
	ctx.f26.f64 = double(temp.f32);
	// lfs f29,8(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 8);
	ctx.f29.f64 = double(temp.f32);
	// fadds f31,f28,f29
	ctx.f31.f64 = double(float(ctx.f28.f64 + ctx.f29.f64));
	// lfs f5,4(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 4);
	ctx.f5.f64 = double(temp.f32);
	// fsubs f2,f24,f2
	ctx.f2.f64 = double(float(ctx.f24.f64 - ctx.f2.f64));
	// lfs f30,4(r6)
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 4);
	ctx.f30.f64 = double(temp.f32);
	// fadds f24,f26,f27
	ctx.f24.f64 = double(float(ctx.f26.f64 + ctx.f27.f64));
	// fsubs f29,f28,f29
	ctx.f29.f64 = double(float(ctx.f28.f64 - ctx.f29.f64));
	// fsubs f28,f26,f27
	ctx.f28.f64 = double(float(ctx.f26.f64 - ctx.f27.f64));
	// lfs f26,0(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 0);
	ctx.f26.f64 = double(temp.f32);
	// fneg f23,f5
	ctx.f23.u64 = ctx.f5.u64 ^ 0x8000000000000000;
	// lfs f27,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f27.f64 = double(temp.f32);
	// fsubs f5,f30,f5
	ctx.f5.f64 = double(float(ctx.f30.f64 - ctx.f5.f64));
	// fadds f22,f31,f25
	ctx.f22.f64 = double(float(ctx.f31.f64 + ctx.f25.f64));
	// fsubs f20,f25,f31
	ctx.f20.f64 = double(float(ctx.f25.f64 - ctx.f31.f64));
	// lfs f31,0(r27)
	temp.u32 = REX_LOAD_U32(ctx.r27.u32 + 0);
	ctx.f31.f64 = double(temp.f32);
	// fsubs f21,f2,f24
	ctx.f21.f64 = double(float(ctx.f2.f64 - ctx.f24.f64));
	// fadds f19,f24,f2
	ctx.f19.f64 = double(float(ctx.f24.f64 + ctx.f2.f64));
	// lfs f24,4(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 4);
	ctx.f24.f64 = double(temp.f32);
	// fadds f2,f29,f6
	ctx.f2.f64 = double(float(ctx.f29.f64 + ctx.f6.f64));
	// stfs f22,8(r3)
	temp.f32 = float(ctx.f22.f64);
	REX_STORE_U32(ctx.r3.u32 + 8, temp.u32);
	// fadds f25,f28,f4
	ctx.f25.f64 = double(float(ctx.f28.f64 + ctx.f4.f64));
	// stfs f21,12(r3)
	temp.f32 = float(ctx.f21.f64);
	REX_STORE_U32(ctx.r3.u32 + 12, temp.u32);
	// fsubs f30,f23,f30
	ctx.f30.f64 = double(float(ctx.f23.f64 - ctx.f30.f64));
	// fsubs f23,f26,f27
	ctx.f23.f64 = double(float(ctx.f26.f64 - ctx.f27.f64));
	// fadds f27,f27,f26
	ctx.f27.f64 = double(float(ctx.f27.f64 + ctx.f26.f64));
	// fmuls f26,f10,f2
	ctx.f26.f64 = double(float(ctx.f10.f64 * ctx.f2.f64));
	// fmr f22,f2
	ctx.f22.f64 = ctx.f2.f64;
	// fmuls f21,f10,f25
	ctx.f21.f64 = double(float(ctx.f10.f64 * ctx.f25.f64));
	// fsubs f2,f24,f31
	ctx.f2.f64 = double(float(ctx.f24.f64 - ctx.f31.f64));
	// fadds f31,f24,f31
	ctx.f31.f64 = double(float(ctx.f24.f64 + ctx.f31.f64));
	// fadds f24,f27,f3
	ctx.f24.f64 = double(float(ctx.f27.f64 + ctx.f3.f64));
	// stfs f24,0(r3)
	temp.f32 = float(ctx.f24.f64);
	REX_STORE_U32(ctx.r3.u32 + 0, temp.u32);
	// fsubs f3,f3,f27
	ctx.f3.f64 = double(float(ctx.f3.f64 - ctx.f27.f64));
	// fadds f10,f23,f5
	ctx.f10.f64 = double(float(ctx.f23.f64 + ctx.f5.f64));
	// fmsubs f27,f9,f25,f26
	ctx.f27.f64 = double(float(std::fma(ctx.f9.f64, ctx.f25.f64, -ctx.f26.f64)));
	// fmadds f26,f9,f22,f21
	ctx.f26.f64 = double(float(std::fma(ctx.f9.f64, ctx.f22.f64, ctx.f21.f64)));
	// fadds f9,f2,f1
	ctx.f9.f64 = double(float(ctx.f2.f64 + ctx.f1.f64));
	// fmuls f25,f0,f10
	ctx.f25.f64 = double(float(ctx.f0.f64 * ctx.f10.f64));
	// addi r28,r28,-1
	ctx.r28.s64 = ctx.r28.s64 + -1;
	// fmr f24,f10
	ctx.f24.f64 = ctx.f10.f64;
	// fsubs f10,f6,f29
	ctx.f10.f64 = double(float(ctx.f6.f64 - ctx.f29.f64));
	// cmplwi cr6,r28,0
	ctx.cr6.compare<uint32_t>(ctx.r28.u32, 0, ctx.xer);
	// fsubs f6,f30,f31
	ctx.f6.f64 = double(float(ctx.f30.f64 - ctx.f31.f64));
	// stfs f6,4(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 4, temp.u32);
	// fadds f6,f31,f30
	ctx.f6.f64 = double(float(ctx.f31.f64 + ctx.f30.f64));
	// stfs f6,4(r5)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r5.u32 + 4, temp.u32);
	// fmr f6,f9
	ctx.f6.f64 = ctx.f9.f64;
	// stfs f3,0(r5)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r5.u32 + 0, temp.u32);
	// fmuls f3,f0,f9
	ctx.f3.f64 = double(float(ctx.f0.f64 * ctx.f9.f64));
	// stfs f20,8(r5)
	temp.f32 = float(ctx.f20.f64);
	REX_STORE_U32(ctx.r5.u32 + 8, temp.u32);
	// fsubs f9,f4,f28
	ctx.f9.f64 = double(float(ctx.f4.f64 - ctx.f28.f64));
	// stfs f19,12(r5)
	temp.f32 = float(ctx.f19.f64);
	REX_STORE_U32(ctx.r5.u32 + 12, temp.u32);
	// stfs f27,8(r6)
	temp.f32 = float(ctx.f27.f64);
	REX_STORE_U32(ctx.r6.u32 + 8, temp.u32);
	// addi r5,r5,-16
	ctx.r5.s64 = ctx.r5.s64 + -16;
	// stfs f26,12(r6)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r6.u32 + 12, temp.u32);
	// addi r3,r3,-16
	ctx.r3.s64 = ctx.r3.s64 + -16;
	// fmr f4,f10
	ctx.f4.f64 = ctx.f10.f64;
	// fmr f31,f10
	ctx.f31.f64 = ctx.f10.f64;
	// fsubs f10,f5,f23
	ctx.f10.f64 = double(float(ctx.f5.f64 - ctx.f23.f64));
	// fmsubs f6,f13,f6,f25
	ctx.f6.f64 = double(float(std::fma(ctx.f13.f64, ctx.f6.f64, -ctx.f25.f64)));
	// stfs f6,0(r6)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r6.u32 + 0, temp.u32);
	// fmadds f6,f13,f24,f3
	ctx.f6.f64 = double(float(std::fma(ctx.f13.f64, ctx.f24.f64, ctx.f3.f64)));
	// stfs f6,4(r6)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r6.u32 + 4, temp.u32);
	// fmuls f6,f7,f9
	ctx.f6.f64 = double(float(ctx.f7.f64 * ctx.f9.f64));
	// addi r6,r6,-16
	ctx.r6.s64 = ctx.r6.s64 + -16;
	// fmuls f5,f8,f9
	ctx.f5.f64 = double(float(ctx.f8.f64 * ctx.f9.f64));
	// fsubs f9,f1,f2
	ctx.f9.f64 = double(float(ctx.f1.f64 - ctx.f2.f64));
	// fmadds f8,f8,f4,f6
	ctx.f8.f64 = double(float(std::fma(ctx.f8.f64, ctx.f4.f64, ctx.f6.f64)));
	// stfs f8,8(r31)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r31.u32 + 8, temp.u32);
	// fmsubs f8,f7,f31,f5
	ctx.f8.f64 = double(float(std::fma(ctx.f7.f64, ctx.f31.f64, -ctx.f5.f64)));
	// stfs f8,12(r31)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r31.u32 + 12, temp.u32);
	// fmuls f8,f11,f9
	ctx.f8.f64 = double(float(ctx.f11.f64 * ctx.f9.f64));
	// fmuls f9,f12,f9
	ctx.f9.f64 = double(float(ctx.f12.f64 * ctx.f9.f64));
	// fmadds f8,f12,f10,f8
	ctx.f8.f64 = double(float(std::fma(ctx.f12.f64, ctx.f10.f64, ctx.f8.f64)));
	// stfs f8,0(r31)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// fmsubs f10,f11,f10,f9
	ctx.f10.f64 = double(float(std::fma(ctx.f11.f64, ctx.f10.f64, -ctx.f9.f64)));
	// addi r31,r31,-16
	ctx.r31.s64 = ctx.r31.s64 + -16;
	// stfs f10,0(r27)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r27.u32 + 0, temp.u32);
	// bne cr6,0x8295b5dc
	if (!ctx.cr6.eq) goto loc_8295B5DC;
loc_8295B90C:
	// rlwinm r10,r29,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// fadds f13,f13,f15
	ctx.fpscr.disableFlushMode();
	ctx.f13.f64 = double(float(ctx.f13.f64 + ctx.f15.f64));
	// add r7,r11,r29
	ctx.r7.u64 = ctx.r11.u64 + ctx.r29.u64;
	// fadds f10,f0,f15
	ctx.f10.f64 = double(float(ctx.f0.f64 + ctx.f15.f64));
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// fsubs f12,f12,f15
	ctx.f12.f64 = double(float(ctx.f12.f64 - ctx.f15.f64));
	// addi r8,r7,-2
	ctx.r8.s64 = ctx.r7.s64 + -2;
	// fsubs f11,f11,f15
	ctx.f11.f64 = double(float(ctx.f11.f64 - ctx.f15.f64));
	// add r6,r7,r11
	ctx.r6.u64 = ctx.r7.u64 + ctx.r11.u64;
	// rlwinm r30,r8,2,0,29
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r8,r6,-2
	ctx.r8.s64 = ctx.r6.s64 + -2;
	// lfs f9,-4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -4);
	ctx.f9.f64 = double(temp.f32);
	// add r5,r6,r11
	ctx.r5.u64 = ctx.r6.u64 + ctx.r11.u64;
	// rlwinm r31,r8,2,0,29
	ctx.r31.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// fneg f2,f9
	ctx.f2.u64 = ctx.f9.u64 ^ 0x8000000000000000;
	// rlwinm r28,r26,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r26.u32 | (ctx.r26.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r11,r6,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// fmuls f0,f13,f17
	ctx.f0.f64 = double(float(ctx.f13.f64 * ctx.f17.f64));
	// rlwinm r9,r7,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// fmuls f13,f10,f17
	ctx.f13.f64 = double(float(ctx.f10.f64 * ctx.f17.f64));
	// rlwinm r8,r5,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 2) & 0xFFFFFFFC;
	// lfsx f8,r30,r4
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + ctx.r4.u32);
	ctx.f8.f64 = double(temp.f32);
	// addi r3,r5,-2
	ctx.r3.s64 = ctx.r5.s64 + -2;
	// lfsx f6,r31,r4
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + ctx.r4.u32);
	ctx.f6.f64 = double(temp.f32);
	// add r11,r11,r4
	ctx.r11.u64 = ctx.r11.u64 + ctx.r4.u64;
	// lfsx f10,r28,r4
	temp.u32 = REX_LOAD_U32(ctx.r28.u32 + ctx.r4.u32);
	ctx.f10.f64 = double(temp.f32);
	// rlwinm r3,r3,2,0,29
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 2) & 0xFFFFFFFC;
	// fadds f3,f10,f6
	ctx.f3.f64 = double(float(ctx.f10.f64 + ctx.f6.f64));
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// fsubs f10,f10,f6
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f6.f64));
	// add r8,r8,r4
	ctx.r8.u64 = ctx.r8.u64 + ctx.r4.u64;
	// fmuls f12,f12,f16
	ctx.f12.f64 = double(float(ctx.f12.f64 * ctx.f16.f64));
	// fmuls f11,f11,f16
	ctx.f11.f64 = double(float(ctx.f11.f64 * ctx.f16.f64));
	// addi r26,r5,2
	ctx.r26.s64 = ctx.r5.s64 + 2;
	// lfs f5,-4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -4);
	ctx.f5.f64 = double(temp.f32);
	// addi r25,r7,2
	ctx.r25.s64 = ctx.r7.s64 + 2;
	// lfsx f4,r3,r4
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + ctx.r4.u32);
	ctx.f4.f64 = double(temp.f32);
	// fsubs f9,f5,f9
	ctx.f9.f64 = double(float(ctx.f5.f64 - ctx.f9.f64));
	// lfs f7,-4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -4);
	ctx.f7.f64 = double(temp.f32);
	// fsubs f2,f2,f5
	ctx.f2.f64 = double(float(ctx.f2.f64 - ctx.f5.f64));
	// lfs f6,-4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + -4);
	ctx.f6.f64 = double(temp.f32);
	// fadds f1,f8,f4
	ctx.f1.f64 = double(float(ctx.f8.f64 + ctx.f4.f64));
	// fadds f5,f7,f6
	ctx.f5.f64 = double(float(ctx.f7.f64 + ctx.f6.f64));
	// fsubs f8,f8,f4
	ctx.f8.f64 = double(float(ctx.f8.f64 - ctx.f4.f64));
	// fsubs f7,f7,f6
	ctx.f7.f64 = double(float(ctx.f7.f64 - ctx.f6.f64));
	// fadds f6,f1,f3
	ctx.f6.f64 = double(float(ctx.f1.f64 + ctx.f3.f64));
	// stfsx f6,r28,r4
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r28.u32 + ctx.r4.u32, temp.u32);
	// fsubs f4,f2,f5
	ctx.f4.f64 = double(float(ctx.f2.f64 - ctx.f5.f64));
	// stfs f4,-4(r10)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r10.u32 + -4, temp.u32);
	// fadds f5,f5,f2
	ctx.f5.f64 = double(float(ctx.f5.f64 + ctx.f2.f64));
	// stfs f5,-4(r9)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r9.u32 + -4, temp.u32);
	// fadds f5,f8,f9
	ctx.f5.f64 = double(float(ctx.f8.f64 + ctx.f9.f64));
	// fsubs f6,f3,f1
	ctx.f6.f64 = double(float(ctx.f3.f64 - ctx.f1.f64));
	// stfsx f6,r30,r4
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r30.u32 + ctx.r4.u32, temp.u32);
	// fadds f6,f7,f10
	ctx.f6.f64 = double(float(ctx.f7.f64 + ctx.f10.f64));
	// fsubs f10,f10,f7
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f7.f64));
	// fsubs f9,f9,f8
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f8.f64));
	// fmuls f4,f0,f5
	ctx.f4.f64 = double(float(ctx.f0.f64 * ctx.f5.f64));
	// fmuls f3,f0,f6
	ctx.f3.f64 = double(float(ctx.f0.f64 * ctx.f6.f64));
	// fmsubs f8,f13,f6,f4
	ctx.f8.f64 = double(float(std::fma(ctx.f13.f64, ctx.f6.f64, -ctx.f4.f64)));
	// stfsx f8,r31,r4
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r31.u32 + ctx.r4.u32, temp.u32);
	// addi r31,r6,2
	ctx.r31.s64 = ctx.r6.s64 + 2;
	// fneg f6,f15
	ctx.f6.u64 = ctx.f15.u64 ^ 0x8000000000000000;
	// fmadds f8,f13,f5,f3
	ctx.f8.f64 = double(float(std::fma(ctx.f13.f64, ctx.f5.f64, ctx.f3.f64)));
	// stfs f8,-4(r11)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r11.u32 + -4, temp.u32);
	// fmuls f8,f12,f10
	ctx.f8.f64 = double(float(ctx.f12.f64 * ctx.f10.f64));
	// addi r6,r6,3
	ctx.r6.s64 = ctx.r6.s64 + 3;
	// fmuls f10,f11,f10
	ctx.f10.f64 = double(float(ctx.f11.f64 * ctx.f10.f64));
	// fmadds f8,f11,f9,f8
	ctx.f8.f64 = double(float(std::fma(ctx.f11.f64, ctx.f9.f64, ctx.f8.f64)));
	// stfsx f8,r3,r4
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r3.u32 + ctx.r4.u32, temp.u32);
	// addi r3,r29,3
	ctx.r3.s64 = ctx.r29.s64 + 3;
	// fmsubs f10,f12,f9,f10
	ctx.f10.f64 = double(float(std::fma(ctx.f12.f64, ctx.f9.f64, -ctx.f10.f64)));
	// stfs f10,-4(r8)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r8.u32 + -4, temp.u32);
	// rlwinm r27,r3,2,0,29
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f10,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f10.f64 = double(temp.f32);
	// addi r3,r29,2
	ctx.r3.s64 = ctx.r29.s64 + 2;
	// lfs f9,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f9.f64 = double(temp.f32);
	// lfs f8,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f8.f64 = double(temp.f32);
	// rlwinm r29,r31,2,0,29
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f7,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f7.f64 = double(temp.f32);
	// rlwinm r28,r3,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 2) & 0xFFFFFFFC;
	// fadds f1,f10,f9
	ctx.f1.f64 = double(float(ctx.f10.f64 + ctx.f9.f64));
	// lfs f5,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f5.f64 = double(temp.f32);
	// fsubs f10,f10,f9
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f9.f64));
	// lfs f4,4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4);
	ctx.f4.f64 = double(temp.f32);
	// fadds f9,f7,f8
	ctx.f9.f64 = double(float(ctx.f7.f64 + ctx.f8.f64));
	// lfs f3,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f3.f64 = double(temp.f32);
	// fneg f31,f5
	ctx.f31.u64 = ctx.f5.u64 ^ 0x8000000000000000;
	// lfs f2,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f2.f64 = double(temp.f32);
	// fsubs f8,f7,f8
	ctx.f8.f64 = double(float(ctx.f7.f64 - ctx.f8.f64));
	// rlwinm r3,r25,2,0,29
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r25.u32 | (ctx.r25.u64 << 32), 2) & 0xFFFFFFFC;
	// fsubs f7,f4,f5
	ctx.f7.f64 = double(float(ctx.f4.f64 - ctx.f5.f64));
	// rlwinm r31,r26,2,0,29
	ctx.r31.u64 = __builtin_rotateleft64(ctx.r26.u32 | (ctx.r26.u64 << 32), 2) & 0xFFFFFFFC;
	// fadds f5,f2,f3
	ctx.f5.f64 = double(float(ctx.f2.f64 + ctx.f3.f64));
	// rlwinm r30,r6,2,0,29
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// fsubs f3,f2,f3
	ctx.f3.f64 = double(float(ctx.f2.f64 - ctx.f3.f64));
	// addi r6,r5,3
	ctx.r6.s64 = ctx.r5.s64 + 3;
	// rlwinm r6,r6,2,0,29
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// fadds f2,f9,f1
	ctx.f2.f64 = double(float(ctx.f9.f64 + ctx.f1.f64));
	// stfs f2,0(r10)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// fsubs f2,f1,f9
	ctx.f2.f64 = double(float(ctx.f1.f64 - ctx.f9.f64));
	// fsubs f9,f31,f4
	ctx.f9.f64 = double(float(ctx.f31.f64 - ctx.f4.f64));
	// fadds f4,f3,f10
	ctx.f4.f64 = double(float(ctx.f3.f64 + ctx.f10.f64));
	// fsubs f10,f10,f3
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f3.f64));
	// fsubs f1,f9,f5
	ctx.f1.f64 = double(float(ctx.f9.f64 - ctx.f5.f64));
	// stfs f1,4(r10)
	temp.f32 = float(ctx.f1.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// fadds f5,f5,f9
	ctx.f5.f64 = double(float(ctx.f5.f64 + ctx.f9.f64));
	// stfs f5,4(r9)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// fadds f9,f8,f7
	ctx.f9.f64 = double(float(ctx.f8.f64 + ctx.f7.f64));
	// stfs f2,0(r9)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// fsubs f5,f4,f9
	ctx.f5.f64 = double(float(ctx.f4.f64 - ctx.f9.f64));
	// fadds f4,f9,f4
	ctx.f4.f64 = double(float(ctx.f9.f64 + ctx.f4.f64));
	// fsubs f9,f7,f8
	ctx.f9.f64 = double(float(ctx.f7.f64 - ctx.f8.f64));
	// fmuls f8,f5,f15
	ctx.f8.f64 = double(float(ctx.f5.f64 * ctx.f15.f64));
	// stfs f8,0(r11)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// fmuls f8,f4,f15
	ctx.f8.f64 = double(float(ctx.f4.f64 * ctx.f15.f64));
	// stfs f8,4(r11)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r11.u32 + 4, temp.u32);
	// fadds f7,f9,f10
	ctx.f7.f64 = double(float(ctx.f9.f64 + ctx.f10.f64));
	// lfsx f8,r31,r4
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + ctx.r4.u32);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f10,f9,f10
	ctx.f10.f64 = double(float(ctx.f9.f64 - ctx.f10.f64));
	// addi r11,r7,3
	ctx.r11.s64 = ctx.r7.s64 + 3;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// fmuls f9,f7,f6
	ctx.f9.f64 = double(float(ctx.f7.f64 * ctx.f6.f64));
	// stfs f9,0(r8)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// fmuls f10,f10,f6
	ctx.f10.f64 = double(float(ctx.f10.f64 * ctx.f6.f64));
	// stfs f10,4(r8)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// lfsx f7,r29,r4
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + ctx.r4.u32);
	ctx.f7.f64 = double(temp.f32);
	// lfsx f9,r28,r4
	temp.u32 = REX_LOAD_U32(ctx.r28.u32 + ctx.r4.u32);
	ctx.f9.f64 = double(temp.f32);
	// lfsx f5,r3,r4
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + ctx.r4.u32);
	ctx.f5.f64 = double(temp.f32);
	// fadds f4,f9,f7
	ctx.f4.f64 = double(float(ctx.f9.f64 + ctx.f7.f64));
	// lfsx f10,r27,r4
	temp.u32 = REX_LOAD_U32(ctx.r27.u32 + ctx.r4.u32);
	ctx.f10.f64 = double(temp.f32);
	// fsubs f9,f9,f7
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f7.f64));
	// fadds f7,f5,f8
	ctx.f7.f64 = double(float(ctx.f5.f64 + ctx.f8.f64));
	// lfsx f6,r30,r4
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + ctx.r4.u32);
	ctx.f6.f64 = double(temp.f32);
	// fneg f3,f10
	ctx.f3.u64 = ctx.f10.u64 ^ 0x8000000000000000;
	// fsubs f8,f5,f8
	ctx.f8.f64 = double(float(ctx.f5.f64 - ctx.f8.f64));
	// fsubs f10,f6,f10
	ctx.f10.f64 = double(float(ctx.f6.f64 - ctx.f10.f64));
	// fadds f5,f7,f4
	ctx.f5.f64 = double(float(ctx.f7.f64 + ctx.f4.f64));
	// fsubs f6,f3,f6
	ctx.f6.f64 = double(float(ctx.f3.f64 - ctx.f6.f64));
	// lfsx f3,r6,r4
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + ctx.r4.u32);
	ctx.f3.f64 = double(temp.f32);
	// fsubs f4,f4,f7
	ctx.f4.f64 = double(float(ctx.f4.f64 - ctx.f7.f64));
	// lfsx f7,r11,r4
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + ctx.r4.u32);
	ctx.f7.f64 = double(temp.f32);
	// stfsx f5,r28,r4
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r28.u32 + ctx.r4.u32, temp.u32);
	// fadds f5,f7,f3
	ctx.f5.f64 = double(float(ctx.f7.f64 + ctx.f3.f64));
	// fsubs f7,f7,f3
	ctx.f7.f64 = double(float(ctx.f7.f64 - ctx.f3.f64));
	// fsubs f3,f6,f5
	ctx.f3.f64 = double(float(ctx.f6.f64 - ctx.f5.f64));
	// stfsx f3,r27,r4
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r27.u32 + ctx.r4.u32, temp.u32);
	// fadds f5,f5,f6
	ctx.f5.f64 = double(float(ctx.f5.f64 + ctx.f6.f64));
	// stfsx f5,r11,r4
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r11.u32 + ctx.r4.u32, temp.u32);
	// fadds f5,f8,f10
	ctx.f5.f64 = double(float(ctx.f8.f64 + ctx.f10.f64));
	// stfsx f4,r3,r4
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r3.u32 + ctx.r4.u32, temp.u32);
	// fadds f6,f7,f9
	ctx.f6.f64 = double(float(ctx.f7.f64 + ctx.f9.f64));
	// fsubs f10,f10,f8
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f8.f64));
	// fmuls f4,f13,f5
	ctx.f4.f64 = double(float(ctx.f13.f64 * ctx.f5.f64));
	// fmuls f3,f13,f6
	ctx.f3.f64 = double(float(ctx.f13.f64 * ctx.f6.f64));
	// fsubs f13,f9,f7
	ctx.f13.f64 = double(float(ctx.f9.f64 - ctx.f7.f64));
	// fmsubs f9,f0,f6,f4
	ctx.f9.f64 = double(float(std::fma(ctx.f0.f64, ctx.f6.f64, -ctx.f4.f64)));
	// stfsx f9,r29,r4
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r29.u32 + ctx.r4.u32, temp.u32);
	// fmadds f0,f0,f5,f3
	ctx.f0.f64 = double(float(std::fma(ctx.f0.f64, ctx.f5.f64, ctx.f3.f64)));
	// stfsx f0,r30,r4
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r30.u32 + ctx.r4.u32, temp.u32);
	// fmuls f0,f11,f13
	ctx.f0.f64 = double(float(ctx.f11.f64 * ctx.f13.f64));
	// fmuls f13,f12,f13
	ctx.f13.f64 = double(float(ctx.f12.f64 * ctx.f13.f64));
	// fmadds f0,f12,f10,f0
	ctx.f0.f64 = double(float(std::fma(ctx.f12.f64, ctx.f10.f64, ctx.f0.f64)));
	// stfsx f0,r31,r4
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + ctx.r4.u32, temp.u32);
	// fmsubs f0,f11,f10,f13
	ctx.f0.f64 = double(float(std::fma(ctx.f11.f64, ctx.f10.f64, -ctx.f13.f64)));
	// stfsx f0,r6,r4
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r6.u32 + ctx.r4.u32, temp.u32);
	// addi r12,r1,-64
	ctx.r12.s64 = ctx.r1.s64 + -64;
	// bl 0x824144dc
	ctx.lr = 0x8295BBAC;
	__restfpr_14(ctx, base);
	// b 0x824131ec
	__restgprlr_25(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295BBB0) {
	REX_FUNC_PROLOGUE(sub_8295BBB0);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a0
	ctx.lr = 0x8295BBB8;
	__savegprlr_26(ctx, base);
	// stfd f31,-64(r1)
	ctx.fpscr.disableFlushMode();
	REX_STORE_U64(ctx.r1.u32 + -64, ctx.f31.u64);
	// srawi r26,r3,3
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0x7) != 0);
	ctx.r26.s64 = ctx.r3.s32 >> 3;
	// lfs f0,0(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,4(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// rlwinm r11,r26,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r26.u32 | (ctx.r26.u64 << 32), 1) & 0xFFFFFFFE;
	// cmpwi cr6,r26,2
	ctx.cr6.compare<int32_t>(ctx.r26.s32, 2, ctx.xer);
	// rlwinm r9,r11,1,0,30
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// rlwinm r10,r11,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r8,r9,r11
	ctx.r8.u64 = ctx.r9.u64 + ctx.r11.u64;
	// rlwinm r9,r9,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r8,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// add r8,r8,r4
	ctx.r8.u64 = ctx.r8.u64 + ctx.r4.u64;
	// lfs f11,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f8,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f8.f64 = double(temp.f32);
	// fadds f7,f0,f11
	ctx.f7.f64 = double(float(ctx.f0.f64 + ctx.f11.f64));
	// lfs f9,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f9.f64 = double(temp.f32);
	// fadds f5,f13,f8
	ctx.f5.f64 = double(float(ctx.f13.f64 + ctx.f8.f64));
	// lfs f12,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// fsubs f0,f0,f11
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f11.f64));
	// fadds f6,f12,f9
	ctx.f6.f64 = double(float(ctx.f12.f64 + ctx.f9.f64));
	// lfs f10,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f10.f64 = double(temp.f32);
	// lfs f11,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f11.f64 = double(temp.f32);
	// fsubs f12,f12,f9
	ctx.f12.f64 = double(float(ctx.f12.f64 - ctx.f9.f64));
	// fsubs f13,f13,f8
	ctx.f13.f64 = double(float(ctx.f13.f64 - ctx.f8.f64));
	// fadds f9,f10,f11
	ctx.f9.f64 = double(float(ctx.f10.f64 + ctx.f11.f64));
	// fsubs f11,f10,f11
	ctx.f11.f64 = double(float(ctx.f10.f64 - ctx.f11.f64));
	// fadds f10,f6,f7
	ctx.f10.f64 = double(float(ctx.f6.f64 + ctx.f7.f64));
	// stfs f10,0(r4)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r4.u32 + 0, temp.u32);
	// fsubs f10,f7,f6
	ctx.f10.f64 = double(float(ctx.f7.f64 - ctx.f6.f64));
	// fadds f8,f12,f13
	ctx.f8.f64 = double(float(ctx.f12.f64 + ctx.f13.f64));
	// fsubs f13,f13,f12
	ctx.f13.f64 = double(float(ctx.f13.f64 - ctx.f12.f64));
	// fadds f12,f9,f5
	ctx.f12.f64 = double(float(ctx.f9.f64 + ctx.f5.f64));
	// stfs f12,4(r4)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r4.u32 + 4, temp.u32);
	// fsubs f12,f5,f9
	ctx.f12.f64 = double(float(ctx.f5.f64 - ctx.f9.f64));
	// stfs f12,4(r10)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// stfs f10,0(r10)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// fsubs f12,f0,f11
	ctx.f12.f64 = double(float(ctx.f0.f64 - ctx.f11.f64));
	// stfs f12,0(r9)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// fadds f0,f11,f0
	ctx.f0.f64 = double(float(ctx.f11.f64 + ctx.f0.f64));
	// stfs f8,4(r9)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// stfs f0,0(r8)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f13,4(r8)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// lfs f31,4(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 4);
	ctx.f31.f64 = double(temp.f32);
	// ble cr6,0x8295be60
	if (!ctx.cr6.gt) goto loc_8295BE60;
	// rlwinm r8,r11,4,0,27
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 4) & 0xFFFFFFF0;
	// rlwinm r10,r11,3,0,28
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 3) & 0xFFFFFFF8;
	// addi r7,r11,2
	ctx.r7.s64 = ctx.r11.s64 + 2;
	// add r8,r8,r4
	ctx.r8.u64 = ctx.r8.u64 + ctx.r4.u64;
	// add r9,r10,r4
	ctx.r9.u64 = ctx.r10.u64 + ctx.r4.u64;
	// rlwinm r10,r7,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r30,r8,-8
	ctx.r30.s64 = ctx.r8.s64 + -8;
	// addi r31,r26,-3
	ctx.r31.s64 = ctx.r26.s64 + -3;
	// rlwinm r8,r11,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// add r7,r10,r4
	ctx.r7.u64 = ctx.r10.u64 + ctx.r4.u64;
	// rlwinm r31,r31,31,1,31
	ctx.r31.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r10,r5,8
	ctx.r10.s64 = ctx.r5.s64 + 8;
	// add r5,r11,r8
	ctx.r5.u64 = ctx.r11.u64 + ctx.r8.u64;
	// addi r29,r31,1
	ctx.r29.s64 = ctx.r31.s64 + 1;
	// rlwinm r31,r5,2,0,29
	ctx.r31.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r6,r11,-2
	ctx.r6.s64 = ctx.r11.s64 + -2;
	// add r31,r31,r4
	ctx.r31.u64 = ctx.r31.u64 + ctx.r4.u64;
	// rlwinm r3,r6,2,0,29
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r8,r9,8
	ctx.r8.s64 = ctx.r9.s64 + 8;
	// addi r5,r9,-8
	ctx.r5.s64 = ctx.r9.s64 + -8;
	// addi r9,r31,8
	ctx.r9.s64 = ctx.r31.s64 + 8;
	// addi r6,r4,8
	ctx.r6.s64 = ctx.r4.s64 + 8;
	// add r3,r3,r4
	ctx.r3.u64 = ctx.r3.u64 + ctx.r4.u64;
	// addi r31,r31,-8
	ctx.r31.s64 = ctx.r31.s64 + -8;
loc_8295BCD0:
	// lfs f0,0(r6)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// addi r10,r10,16
	ctx.r10.s64 = ctx.r10.s64 + 16;
	// lfs f13,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// addi r28,r31,4
	ctx.r28.s64 = ctx.r31.s64 + 4;
	// lfs f12,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// fadds f8,f13,f0
	ctx.f8.f64 = double(float(ctx.f13.f64 + ctx.f0.f64));
	// lfs f11,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fsubs f6,f0,f13
	ctx.f6.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// lfs f9,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f9.f64 = double(temp.f32);
	// fadds f4,f12,f11
	ctx.f4.f64 = double(float(ctx.f12.f64 + ctx.f11.f64));
	// lfs f10,4(r6)
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 4);
	ctx.f10.f64 = double(temp.f32);
	// fsubs f3,f11,f12
	ctx.f3.f64 = double(float(ctx.f11.f64 - ctx.f12.f64));
	// lfs f5,4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 4);
	ctx.f5.f64 = double(temp.f32);
	// fadds f2,f9,f10
	ctx.f2.f64 = double(float(ctx.f9.f64 + ctx.f10.f64));
	// lfs f7,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f7.f64 = double(temp.f32);
	// fsubs f10,f10,f9
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f9.f64));
	// fadds f9,f7,f5
	ctx.f9.f64 = double(float(ctx.f7.f64 + ctx.f5.f64));
	// lfs f0,-4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -4);
	ctx.f0.f64 = double(temp.f32);
	// fsubs f7,f5,f7
	ctx.f7.f64 = double(float(ctx.f5.f64 - ctx.f7.f64));
	// lfs f13,-8(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -8);
	ctx.f13.f64 = double(temp.f32);
	// lfs f11,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// addi r27,r30,4
	ctx.r27.s64 = ctx.r30.s64 + 4;
	// lfs f12,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// addi r29,r29,-1
	ctx.r29.s64 = ctx.r29.s64 + -1;
	// fneg f12,f12
	ctx.f12.u64 = ctx.f12.u64 ^ 0x8000000000000000;
	// fadds f5,f4,f8
	ctx.f5.f64 = double(float(ctx.f4.f64 + ctx.f8.f64));
	// stfs f5,0(r6)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r6.u32 + 0, temp.u32);
	// fsubs f8,f8,f4
	ctx.f8.f64 = double(float(ctx.f8.f64 - ctx.f4.f64));
	// fadds f5,f9,f2
	ctx.f5.f64 = double(float(ctx.f9.f64 + ctx.f2.f64));
	// stfs f5,4(r6)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r6.u32 + 4, temp.u32);
	// fsubs f5,f2,f9
	ctx.f5.f64 = double(float(ctx.f2.f64 - ctx.f9.f64));
	// stfs f8,0(r7)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// fsubs f9,f6,f7
	ctx.f9.f64 = double(float(ctx.f6.f64 - ctx.f7.f64));
	// stfs f5,4(r7)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r7.u32 + 4, temp.u32);
	// fadds f8,f3,f10
	ctx.f8.f64 = double(float(ctx.f3.f64 + ctx.f10.f64));
	// addi r7,r7,8
	ctx.r7.s64 = ctx.r7.s64 + 8;
	// fsubs f10,f10,f3
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f3.f64));
	// addi r6,r6,8
	ctx.r6.s64 = ctx.r6.s64 + 8;
	// fmuls f2,f0,f9
	ctx.f2.f64 = double(float(ctx.f0.f64 * ctx.f9.f64));
	// fmr f5,f9
	ctx.f5.f64 = ctx.f9.f64;
	// fadds f9,f7,f6
	ctx.f9.f64 = double(float(ctx.f7.f64 + ctx.f6.f64));
	// fmuls f4,f0,f8
	ctx.f4.f64 = double(float(ctx.f0.f64 * ctx.f8.f64));
	// fmadds f8,f13,f8,f2
	ctx.f8.f64 = double(float(std::fma(ctx.f13.f64, ctx.f8.f64, ctx.f2.f64)));
	// stfs f8,4(r8)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// fmuls f8,f11,f9
	ctx.f8.f64 = double(float(ctx.f11.f64 * ctx.f9.f64));
	// fmuls f9,f12,f9
	ctx.f9.f64 = double(float(ctx.f12.f64 * ctx.f9.f64));
	// fmsubs f7,f13,f5,f4
	ctx.f7.f64 = double(float(std::fma(ctx.f13.f64, ctx.f5.f64, -ctx.f4.f64)));
	// stfs f7,0(r8)
	temp.f32 = float(ctx.f7.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// addi r8,r8,8
	ctx.r8.s64 = ctx.r8.s64 + 8;
	// fmadds f8,f12,f10,f8
	ctx.f8.f64 = double(float(std::fma(ctx.f12.f64, ctx.f10.f64, ctx.f8.f64)));
	// stfs f8,0(r9)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// fmsubs f10,f11,f10,f9
	ctx.f10.f64 = double(float(std::fma(ctx.f11.f64, ctx.f10.f64, -ctx.f9.f64)));
	// stfs f10,4(r9)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// lfs f9,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f9.f64 = double(temp.f32);
	// addi r9,r9,8
	ctx.r9.s64 = ctx.r9.s64 + 8;
	// lfs f10,0(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 0);
	ctx.f10.f64 = double(temp.f32);
	// lfs f7,0(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 0);
	ctx.f7.f64 = double(temp.f32);
	// fadds f2,f9,f10
	ctx.f2.f64 = double(float(ctx.f9.f64 + ctx.f10.f64));
	// lfs f8,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f10,f10,f9
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f9.f64));
	// lfs f4,4(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 4);
	ctx.f4.f64 = double(temp.f32);
	// fadds f9,f8,f7
	ctx.f9.f64 = double(float(ctx.f8.f64 + ctx.f7.f64));
	// lfs f6,0(r28)
	temp.u32 = REX_LOAD_U32(ctx.r28.u32 + 0);
	ctx.f6.f64 = double(temp.f32);
	// fsubs f8,f7,f8
	ctx.f8.f64 = double(float(ctx.f7.f64 - ctx.f8.f64));
	// lfs f5,0(r27)
	temp.u32 = REX_LOAD_U32(ctx.r27.u32 + 0);
	ctx.f5.f64 = double(temp.f32);
	// lfs f3,4(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 4);
	ctx.f3.f64 = double(temp.f32);
	// fadds f7,f5,f4
	ctx.f7.f64 = double(float(ctx.f5.f64 + ctx.f4.f64));
	// fadds f1,f6,f3
	ctx.f1.f64 = double(float(ctx.f6.f64 + ctx.f3.f64));
	// fsubs f5,f4,f5
	ctx.f5.f64 = double(float(ctx.f4.f64 - ctx.f5.f64));
	// fsubs f6,f3,f6
	ctx.f6.f64 = double(float(ctx.f3.f64 - ctx.f6.f64));
	// fadds f4,f9,f2
	ctx.f4.f64 = double(float(ctx.f9.f64 + ctx.f2.f64));
	// stfs f4,0(r3)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r3.u32 + 0, temp.u32);
	// fsubs f3,f2,f9
	ctx.f3.f64 = double(float(ctx.f2.f64 - ctx.f9.f64));
	// fadds f4,f7,f1
	ctx.f4.f64 = double(float(ctx.f7.f64 + ctx.f1.f64));
	// stfs f4,4(r3)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r3.u32 + 4, temp.u32);
	// fsubs f2,f1,f7
	ctx.f2.f64 = double(float(ctx.f1.f64 - ctx.f7.f64));
	// stfs f3,0(r5)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r5.u32 + 0, temp.u32);
	// stfs f2,4(r5)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r5.u32 + 4, temp.u32);
	// fsubs f9,f10,f5
	ctx.f9.f64 = double(float(ctx.f10.f64 - ctx.f5.f64));
	// fadds f7,f8,f6
	ctx.f7.f64 = double(float(ctx.f8.f64 + ctx.f6.f64));
	// addi r5,r5,-8
	ctx.r5.s64 = ctx.r5.s64 + -8;
	// fmuls f4,f13,f7
	ctx.f4.f64 = double(float(ctx.f13.f64 * ctx.f7.f64));
	// addi r3,r3,-8
	ctx.r3.s64 = ctx.r3.s64 + -8;
	// fmuls f3,f13,f9
	ctx.f3.f64 = double(float(ctx.f13.f64 * ctx.f9.f64));
	// cmplwi cr6,r29,0
	ctx.cr6.compare<uint32_t>(ctx.r29.u32, 0, ctx.xer);
	// fadds f13,f5,f10
	ctx.f13.f64 = double(float(ctx.f5.f64 + ctx.f10.f64));
	// fsubs f10,f6,f8
	ctx.f10.f64 = double(float(ctx.f6.f64 - ctx.f8.f64));
	// fmsubs f9,f0,f9,f4
	ctx.f9.f64 = double(float(std::fma(ctx.f0.f64, ctx.f9.f64, -ctx.f4.f64)));
	// stfs f9,0(r31)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// fmadds f0,f0,f7,f3
	ctx.f0.f64 = double(float(std::fma(ctx.f0.f64, ctx.f7.f64, ctx.f3.f64)));
	// stfs f0,0(r28)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r28.u32 + 0, temp.u32);
	// fmuls f0,f12,f13
	ctx.f0.f64 = double(float(ctx.f12.f64 * ctx.f13.f64));
	// addi r31,r31,-8
	ctx.r31.s64 = ctx.r31.s64 + -8;
	// fmuls f13,f11,f13
	ctx.f13.f64 = double(float(ctx.f11.f64 * ctx.f13.f64));
	// fmadds f0,f11,f10,f0
	ctx.f0.f64 = double(float(std::fma(ctx.f11.f64, ctx.f10.f64, ctx.f0.f64)));
	// stfs f0,0(r30)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// fmsubs f0,f12,f10,f13
	ctx.f0.f64 = double(float(std::fma(ctx.f12.f64, ctx.f10.f64, -ctx.f13.f64)));
	// addi r30,r30,-8
	ctx.r30.s64 = ctx.r30.s64 + -8;
	// stfs f0,0(r27)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r27.u32 + 0, temp.u32);
	// bne cr6,0x8295bcd0
	if (!ctx.cr6.eq) goto loc_8295BCD0;
loc_8295BE60:
	// add r9,r11,r26
	ctx.r9.u64 = ctx.r11.u64 + ctx.r26.u64;
	// fneg f0,f31
	ctx.fpscr.disableFlushMode();
	ctx.f0.u64 = ctx.f31.u64 ^ 0x8000000000000000;
	// rlwinm r10,r26,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r26.u32 | (ctx.r26.u64 << 32), 2) & 0xFFFFFFFC;
	// add r8,r9,r11
	ctx.r8.u64 = ctx.r9.u64 + ctx.r11.u64;
	// rlwinm r9,r9,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// add r7,r8,r11
	ctx.r7.u64 = ctx.r8.u64 + ctx.r11.u64;
	// rlwinm r11,r8,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r7,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// add r11,r11,r4
	ctx.r11.u64 = ctx.r11.u64 + ctx.r4.u64;
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// add r8,r8,r4
	ctx.r8.u64 = ctx.r8.u64 + ctx.r4.u64;
	// lfs f13,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// lfs f10,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f10.f64 = double(temp.f32);
	// lfs f12,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// fadds f6,f13,f10
	ctx.f6.f64 = double(float(ctx.f13.f64 + ctx.f10.f64));
	// lfs f8,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f13,f13,f10
	ctx.f13.f64 = double(float(ctx.f13.f64 - ctx.f10.f64));
	// lfs f9,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f9.f64 = double(temp.f32);
	// fadds f5,f12,f8
	ctx.f5.f64 = double(float(ctx.f12.f64 + ctx.f8.f64));
	// lfs f7,4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4);
	ctx.f7.f64 = double(temp.f32);
	// fsubs f12,f12,f8
	ctx.f12.f64 = double(float(ctx.f12.f64 - ctx.f8.f64));
	// lfs f11,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f11.f64 = double(temp.f32);
	// lfs f10,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f10.f64 = double(temp.f32);
	// fadds f4,f11,f7
	ctx.f4.f64 = double(float(ctx.f11.f64 + ctx.f7.f64));
	// fadds f8,f9,f10
	ctx.f8.f64 = double(float(ctx.f9.f64 + ctx.f10.f64));
	// fsubs f11,f11,f7
	ctx.f11.f64 = double(float(ctx.f11.f64 - ctx.f7.f64));
	// fsubs f10,f9,f10
	ctx.f10.f64 = double(float(ctx.f9.f64 - ctx.f10.f64));
	// fadds f9,f5,f6
	ctx.f9.f64 = double(float(ctx.f5.f64 + ctx.f6.f64));
	// stfs f9,0(r10)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// fsubs f9,f6,f5
	ctx.f9.f64 = double(float(ctx.f6.f64 - ctx.f5.f64));
	// fadds f7,f8,f4
	ctx.f7.f64 = double(float(ctx.f8.f64 + ctx.f4.f64));
	// stfs f7,4(r10)
	temp.f32 = float(ctx.f7.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// fsubs f8,f4,f8
	ctx.f8.f64 = double(float(ctx.f4.f64 - ctx.f8.f64));
	// stfs f9,0(r9)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// stfs f8,4(r9)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// fadds f8,f12,f11
	ctx.f8.f64 = double(float(ctx.f12.f64 + ctx.f11.f64));
	// fsubs f9,f13,f10
	ctx.f9.f64 = double(float(ctx.f13.f64 - ctx.f10.f64));
	// fsubs f12,f11,f12
	ctx.f12.f64 = double(float(ctx.f11.f64 - ctx.f12.f64));
	// fadds f13,f10,f13
	ctx.f13.f64 = double(float(ctx.f10.f64 + ctx.f13.f64));
	// fsubs f7,f9,f8
	ctx.f7.f64 = double(float(ctx.f9.f64 - ctx.f8.f64));
	// fadds f9,f8,f9
	ctx.f9.f64 = double(float(ctx.f8.f64 + ctx.f9.f64));
	// fmuls f11,f7,f31
	ctx.f11.f64 = double(float(ctx.f7.f64 * ctx.f31.f64));
	// stfs f11,0(r11)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// fmuls f11,f9,f31
	ctx.f11.f64 = double(float(ctx.f9.f64 * ctx.f31.f64));
	// stfs f11,4(r11)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r11.u32 + 4, temp.u32);
	// fadds f11,f12,f13
	ctx.f11.f64 = double(float(ctx.f12.f64 + ctx.f13.f64));
	// fsubs f13,f12,f13
	ctx.f13.f64 = double(float(ctx.f12.f64 - ctx.f13.f64));
	// fmuls f12,f11,f0
	ctx.f12.f64 = double(float(ctx.f11.f64 * ctx.f0.f64));
	// stfs f12,0(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// fmuls f0,f13,f0
	ctx.f0.f64 = double(float(ctx.f13.f64 * ctx.f0.f64));
	// stfs f0,4(r8)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// lfd f31,-64(r1)
	ctx.f31.u64 = REX_LOAD_U64(ctx.r1.u32 + -64);
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295BF38) {
	REX_FUNC_PROLOGUE(sub_8295BF38);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413194
	ctx.lr = 0x8295BF40;
	__savegprlr_23(ctx, base);
	// addi r12,r1,-80
	ctx.r12.s64 = ctx.r1.s64 + -80;
	// bl 0x824144bc
	ctx.lr = 0x8295BF48;
	__savefpr_25(ctx, base);
	// srawi r24,r3,3
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0x7) != 0);
	ctx.r24.s64 = ctx.r3.s32 >> 3;
	// lfs f12,4(r4)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// lfs f13,0(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// rlwinm r11,r24,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 1) & 0xFFFFFFFE;
	// lfs f0,4(r5)
	temp.u32 = REX_LOAD_U32(ctx.r5.u32 + 4);
	ctx.f0.f64 = double(temp.f32);
	// cmpwi cr6,r24,2
	ctx.cr6.compare<int32_t>(ctx.r24.s32, 2, ctx.xer);
	// rlwinm r9,r11,1,0,30
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// rlwinm r23,r11,2,0,29
	ctx.r23.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r8,r9,r11
	ctx.r8.u64 = ctx.r9.u64 + ctx.r11.u64;
	// rlwinm r9,r9,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r8,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r23,r4
	ctx.r10.u64 = ctx.r23.u64 + ctx.r4.u64;
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// add r8,r8,r4
	ctx.r8.u64 = ctx.r8.u64 + ctx.r4.u64;
	// rlwinm r7,r11,1,0,30
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// lfs f9,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f9.f64 = double(temp.f32);
	// lfs f10,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f10.f64 = double(temp.f32);
	// lfs f8,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f8.f64 = double(temp.f32);
	// fadds f6,f12,f10
	ctx.f6.f64 = double(float(ctx.f12.f64 + ctx.f10.f64));
	// lfs f7,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f7.f64 = double(temp.f32);
	// fadds f5,f8,f9
	ctx.f5.f64 = double(float(ctx.f8.f64 + ctx.f9.f64));
	// fsubs f12,f12,f10
	ctx.f12.f64 = double(float(ctx.f12.f64 - ctx.f10.f64));
	// lfs f10,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f10.f64 = double(temp.f32);
	// fsubs f9,f9,f8
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f8.f64));
	// lfs f11,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fsubs f8,f13,f7
	ctx.f8.f64 = double(float(ctx.f13.f64 - ctx.f7.f64));
	// fadds f13,f7,f13
	ctx.f13.f64 = double(float(ctx.f7.f64 + ctx.f13.f64));
	// fsubs f7,f11,f10
	ctx.f7.f64 = double(float(ctx.f11.f64 - ctx.f10.f64));
	// fadds f11,f10,f11
	ctx.f11.f64 = double(float(ctx.f10.f64 + ctx.f11.f64));
	// fsubs f10,f7,f5
	ctx.f10.f64 = double(float(ctx.f7.f64 - ctx.f5.f64));
	// fadds f7,f5,f7
	ctx.f7.f64 = double(float(ctx.f5.f64 + ctx.f7.f64));
	// fsubs f5,f11,f9
	ctx.f5.f64 = double(float(ctx.f11.f64 - ctx.f9.f64));
	// fadds f9,f9,f11
	ctx.f9.f64 = double(float(ctx.f9.f64 + ctx.f11.f64));
	// fmuls f11,f10,f0
	ctx.f11.f64 = double(float(ctx.f10.f64 * ctx.f0.f64));
	// fmuls f10,f7,f0
	ctx.f10.f64 = double(float(ctx.f7.f64 * ctx.f0.f64));
	// fadds f7,f11,f8
	ctx.f7.f64 = double(float(ctx.f11.f64 + ctx.f8.f64));
	// stfs f7,0(r4)
	temp.f32 = float(ctx.f7.f64);
	REX_STORE_U32(ctx.r4.u32 + 0, temp.u32);
	// fadds f7,f10,f6
	ctx.f7.f64 = double(float(ctx.f10.f64 + ctx.f6.f64));
	// stfs f7,4(r4)
	temp.f32 = float(ctx.f7.f64);
	REX_STORE_U32(ctx.r4.u32 + 4, temp.u32);
	// fsubs f11,f8,f11
	ctx.f11.f64 = double(float(ctx.f8.f64 - ctx.f11.f64));
	// stfs f11,0(r10)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// fsubs f11,f6,f10
	ctx.f11.f64 = double(float(ctx.f6.f64 - ctx.f10.f64));
	// stfs f11,4(r10)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// fmuls f11,f5,f0
	ctx.f11.f64 = double(float(ctx.f5.f64 * ctx.f0.f64));
	// fmuls f0,f9,f0
	ctx.f0.f64 = double(float(ctx.f9.f64 * ctx.f0.f64));
	// fadds f10,f11,f12
	ctx.f10.f64 = double(float(ctx.f11.f64 + ctx.f12.f64));
	// stfs f10,4(r9)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// fsubs f10,f13,f0
	ctx.f10.f64 = double(float(ctx.f13.f64 - ctx.f0.f64));
	// stfs f10,0(r9)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// fadds f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f13.f64));
	// stfs f0,0(r8)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// fsubs f0,f12,f11
	ctx.f0.f64 = double(float(ctx.f12.f64 - ctx.f11.f64));
	// stfs f0,4(r8)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// ble cr6,0x8295c270
	if (!ctx.cr6.gt) goto loc_8295C270;
	// rlwinm r10,r11,3,0,28
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 3) & 0xFFFFFFF8;
	// addi r3,r24,-3
	ctx.r3.s64 = ctx.r24.s64 + -3;
	// add r8,r10,r4
	ctx.r8.u64 = ctx.r10.u64 + ctx.r4.u64;
	// addi r10,r7,2
	ctx.r10.s64 = ctx.r7.s64 + 2;
	// rlwinm r7,r11,4,0,27
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 4) & 0xFFFFFFF0;
	// rlwinm r31,r3,31,1,31
	ctx.r31.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 31) & 0x7FFFFFFF;
	// add r7,r7,r4
	ctx.r7.u64 = ctx.r7.u64 + ctx.r4.u64;
	// addi r6,r11,-2
	ctx.r6.s64 = ctx.r11.s64 + -2;
	// addi r28,r7,-8
	ctx.r28.s64 = ctx.r7.s64 + -8;
	// rlwinm r7,r11,1,0,30
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// rlwinm r30,r6,2,0,29
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r27,r31,1
	ctx.r27.s64 = ctx.r31.s64 + 1;
	// add r31,r11,r7
	ctx.r31.u64 = ctx.r11.u64 + ctx.r7.u64;
	// add r29,r30,r4
	ctx.r29.u64 = ctx.r30.u64 + ctx.r4.u64;
	// addi r9,r11,2
	ctx.r9.s64 = ctx.r11.s64 + 2;
	// rlwinm r30,r31,2,0,29
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r9,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// add r30,r30,r4
	ctx.r30.u64 = ctx.r30.u64 + ctx.r4.u64;
	// add r6,r9,r4
	ctx.r6.u64 = ctx.r9.u64 + ctx.r4.u64;
	// addi r7,r8,8
	ctx.r7.s64 = ctx.r8.s64 + 8;
	// addi r31,r8,-8
	ctx.r31.s64 = ctx.r8.s64 + -8;
	// add r9,r10,r5
	ctx.r9.u64 = ctx.r10.u64 + ctx.r5.u64;
	// addi r8,r30,8
	ctx.r8.s64 = ctx.r30.s64 + 8;
	// addi r3,r4,8
	ctx.r3.s64 = ctx.r4.s64 + 8;
	// addi r10,r5,8
	ctx.r10.s64 = ctx.r5.s64 + 8;
	// addi r30,r30,-8
	ctx.r30.s64 = ctx.r30.s64 + -8;
loc_8295C08C:
	// lfs f1,4(r3)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 4);
	ctx.f1.f64 = double(temp.f32);
	// addi r10,r10,16
	ctx.r10.s64 = ctx.r10.s64 + 16;
	// lfs f5,0(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 0);
	ctx.f5.f64 = double(temp.f32);
	// addi r9,r9,-16
	ctx.r9.s64 = ctx.r9.s64 + -16;
	// lfs f2,4(r7)
	temp.u32 = REX_LOAD_U32(ctx.r7.u32 + 4);
	ctx.f2.f64 = double(temp.f32);
	// fadds f28,f5,f1
	ctx.f28.f64 = double(float(ctx.f5.f64 + ctx.f1.f64));
	// lfs f6,0(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 0);
	ctx.f6.f64 = double(temp.f32);
	// fsubs f5,f1,f5
	ctx.f5.f64 = double(float(ctx.f1.f64 - ctx.f5.f64));
	// lfs f30,4(r6)
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 4);
	ctx.f30.f64 = double(temp.f32);
	// fsubs f29,f6,f2
	ctx.f29.f64 = double(float(ctx.f6.f64 - ctx.f2.f64));
	// lfs f3,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f3.f64 = double(temp.f32);
	// fadds f6,f2,f6
	ctx.f6.f64 = double(float(ctx.f2.f64 + ctx.f6.f64));
	// fadds f26,f3,f30
	ctx.f26.f64 = double(float(ctx.f3.f64 + ctx.f30.f64));
	// lfs f31,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f31.f64 = double(temp.f32);
	// lfs f4,0(r6)
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 0);
	ctx.f4.f64 = double(temp.f32);
	// fsubs f3,f30,f3
	ctx.f3.f64 = double(float(ctx.f30.f64 - ctx.f3.f64));
	// fsubs f27,f4,f31
	ctx.f27.f64 = double(float(ctx.f4.f64 - ctx.f31.f64));
	// lfs f13,-4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f10,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f10.f64 = double(temp.f32);
	// fadds f4,f31,f4
	ctx.f4.f64 = double(float(ctx.f31.f64 + ctx.f4.f64));
	// lfs f12,-8(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -8);
	ctx.f12.f64 = double(temp.f32);
	// fneg f10,f10
	ctx.f10.u64 = ctx.f10.u64 ^ 0x8000000000000000;
	// lfs f8,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f8.f64 = double(temp.f32);
	// addi r26,r29,4
	ctx.r26.s64 = ctx.r29.s64 + 4;
	// fmuls f2,f13,f28
	ctx.f2.f64 = double(float(ctx.f13.f64 * ctx.f28.f64));
	// lfs f0,-8(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -8);
	ctx.f0.f64 = double(temp.f32);
	// fneg f8,f8
	ctx.f8.u64 = ctx.f8.u64 ^ 0x8000000000000000;
	// lfs f9,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f9.f64 = double(temp.f32);
	// fmuls f1,f13,f29
	ctx.f1.f64 = double(float(ctx.f13.f64 * ctx.f29.f64));
	// lfs f11,-4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + -4);
	ctx.f11.f64 = double(temp.f32);
	// fmuls f25,f9,f6
	ctx.f25.f64 = double(float(ctx.f9.f64 * ctx.f6.f64));
	// lfs f7,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f7.f64 = double(temp.f32);
	// fmuls f31,f12,f26
	ctx.f31.f64 = double(float(ctx.f12.f64 * ctx.f26.f64));
	// addi r25,r28,4
	ctx.r25.s64 = ctx.r28.s64 + 4;
	// addi r27,r27,-1
	ctx.r27.s64 = ctx.r27.s64 + -1;
	// fmuls f30,f12,f27
	ctx.f30.f64 = double(float(ctx.f12.f64 * ctx.f27.f64));
	// cmplwi cr6,r27,0
	ctx.cr6.compare<uint32_t>(ctx.r27.u32, 0, ctx.xer);
	// fmsubs f2,f0,f29,f2
	ctx.f2.f64 = double(float(std::fma(ctx.f0.f64, ctx.f29.f64, -ctx.f2.f64)));
	// fmuls f29,f10,f6
	ctx.f29.f64 = double(float(ctx.f10.f64 * ctx.f6.f64));
	// fmadds f6,f0,f28,f1
	ctx.f6.f64 = double(float(std::fma(ctx.f0.f64, ctx.f28.f64, ctx.f1.f64)));
	// fmuls f28,f8,f4
	ctx.f28.f64 = double(float(ctx.f8.f64 * ctx.f4.f64));
	// fmsubs f1,f11,f27,f31
	ctx.f1.f64 = double(float(std::fma(ctx.f11.f64, ctx.f27.f64, -ctx.f31.f64)));
	// fmadds f31,f11,f26,f30
	ctx.f31.f64 = double(float(std::fma(ctx.f11.f64, ctx.f26.f64, ctx.f30.f64)));
	// fmuls f30,f7,f4
	ctx.f30.f64 = double(float(ctx.f7.f64 * ctx.f4.f64));
	// fadds f4,f1,f2
	ctx.f4.f64 = double(float(ctx.f1.f64 + ctx.f2.f64));
	// stfs f4,0(r3)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r3.u32 + 0, temp.u32);
	// fsubs f4,f2,f1
	ctx.f4.f64 = double(float(ctx.f2.f64 - ctx.f1.f64));
	// fadds f2,f31,f6
	ctx.f2.f64 = double(float(ctx.f31.f64 + ctx.f6.f64));
	// stfs f2,4(r3)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r3.u32 + 4, temp.u32);
	// stfs f4,0(r6)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r6.u32 + 0, temp.u32);
	// fsubs f2,f6,f31
	ctx.f2.f64 = double(float(ctx.f6.f64 - ctx.f31.f64));
	// fmadds f4,f7,f3,f28
	ctx.f4.f64 = double(float(std::fma(ctx.f7.f64, ctx.f3.f64, ctx.f28.f64)));
	// stfs f2,4(r6)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r6.u32 + 4, temp.u32);
	// fmadds f6,f10,f5,f25
	ctx.f6.f64 = double(float(std::fma(ctx.f10.f64, ctx.f5.f64, ctx.f25.f64)));
	// addi r6,r6,8
	ctx.r6.s64 = ctx.r6.s64 + 8;
	// fmsubs f5,f9,f5,f29
	ctx.f5.f64 = double(float(std::fma(ctx.f9.f64, ctx.f5.f64, -ctx.f29.f64)));
	// addi r3,r3,8
	ctx.r3.s64 = ctx.r3.s64 + 8;
	// fmsubs f3,f8,f3,f30
	ctx.f3.f64 = double(float(std::fma(ctx.f8.f64, ctx.f3.f64, -ctx.f30.f64)));
	// fadds f2,f4,f6
	ctx.f2.f64 = double(float(ctx.f4.f64 + ctx.f6.f64));
	// stfs f2,0(r7)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r7.u32 + 0, temp.u32);
	// fsubs f6,f6,f4
	ctx.f6.f64 = double(float(ctx.f6.f64 - ctx.f4.f64));
	// fadds f2,f3,f5
	ctx.f2.f64 = double(float(ctx.f3.f64 + ctx.f5.f64));
	// stfs f2,4(r7)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r7.u32 + 4, temp.u32);
	// stfs f6,0(r8)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// fsubs f6,f5,f3
	ctx.f6.f64 = double(float(ctx.f5.f64 - ctx.f3.f64));
	// stfs f6,4(r8)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// addi r7,r7,8
	ctx.r7.s64 = ctx.r7.s64 + 8;
	// lfs f6,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f6.f64 = double(temp.f32);
	// addi r8,r8,8
	ctx.r8.s64 = ctx.r8.s64 + 8;
	// lfs f2,4(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 4);
	ctx.f2.f64 = double(temp.f32);
	// lfs f5,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f5.f64 = double(temp.f32);
	// fsubs f29,f6,f2
	ctx.f29.f64 = double(float(ctx.f6.f64 - ctx.f2.f64));
	// lfs f4,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f4.f64 = double(temp.f32);
	// fadds f6,f2,f6
	ctx.f6.f64 = double(float(ctx.f2.f64 + ctx.f6.f64));
	// lfs f3,0(r28)
	temp.u32 = REX_LOAD_U32(ctx.r28.u32 + 0);
	ctx.f3.f64 = double(temp.f32);
	// lfs f1,0(r26)
	temp.u32 = REX_LOAD_U32(ctx.r26.u32 + 0);
	ctx.f1.f64 = double(temp.f32);
	// lfs f31,0(r25)
	temp.u32 = REX_LOAD_U32(ctx.r25.u32 + 0);
	ctx.f31.f64 = double(temp.f32);
	// fadds f28,f5,f1
	ctx.f28.f64 = double(float(ctx.f5.f64 + ctx.f1.f64));
	// lfs f30,4(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 4);
	ctx.f30.f64 = double(temp.f32);
	// fsubs f27,f4,f31
	ctx.f27.f64 = double(float(ctx.f4.f64 - ctx.f31.f64));
	// fadds f26,f3,f30
	ctx.f26.f64 = double(float(ctx.f3.f64 + ctx.f30.f64));
	// fmuls f2,f11,f28
	ctx.f2.f64 = double(float(ctx.f11.f64 * ctx.f28.f64));
	// fsubs f5,f1,f5
	ctx.f5.f64 = double(float(ctx.f1.f64 - ctx.f5.f64));
	// fmuls f11,f11,f29
	ctx.f11.f64 = double(float(ctx.f11.f64 * ctx.f29.f64));
	// fmuls f1,f0,f26
	ctx.f1.f64 = double(float(ctx.f0.f64 * ctx.f26.f64));
	// fadds f4,f31,f4
	ctx.f4.f64 = double(float(ctx.f31.f64 + ctx.f4.f64));
	// fmuls f31,f0,f27
	ctx.f31.f64 = double(float(ctx.f0.f64 * ctx.f27.f64));
	// fsubs f3,f30,f3
	ctx.f3.f64 = double(float(ctx.f30.f64 - ctx.f3.f64));
	// fmuls f30,f7,f6
	ctx.f30.f64 = double(float(ctx.f7.f64 * ctx.f6.f64));
	// fmuls f6,f8,f6
	ctx.f6.f64 = double(float(ctx.f8.f64 * ctx.f6.f64));
	// fmsubs f0,f12,f29,f2
	ctx.f0.f64 = double(float(std::fma(ctx.f12.f64, ctx.f29.f64, -ctx.f2.f64)));
	// fmadds f12,f12,f28,f11
	ctx.f12.f64 = double(float(std::fma(ctx.f12.f64, ctx.f28.f64, ctx.f11.f64)));
	// fmsubs f11,f13,f27,f1
	ctx.f11.f64 = double(float(std::fma(ctx.f13.f64, ctx.f27.f64, -ctx.f1.f64)));
	// fmuls f2,f10,f4
	ctx.f2.f64 = double(float(ctx.f10.f64 * ctx.f4.f64));
	// fmadds f13,f13,f26,f31
	ctx.f13.f64 = double(float(std::fma(ctx.f13.f64, ctx.f26.f64, ctx.f31.f64)));
	// fmuls f4,f9,f4
	ctx.f4.f64 = double(float(ctx.f9.f64 * ctx.f4.f64));
	// fadds f1,f11,f0
	ctx.f1.f64 = double(float(ctx.f11.f64 + ctx.f0.f64));
	// stfs f1,0(r29)
	temp.f32 = float(ctx.f1.f64);
	REX_STORE_U32(ctx.r29.u32 + 0, temp.u32);
	// fsubs f0,f0,f11
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f11.f64));
	// addi r29,r29,-8
	ctx.r29.s64 = ctx.r29.s64 + -8;
	// fadds f11,f13,f12
	ctx.f11.f64 = double(float(ctx.f13.f64 + ctx.f12.f64));
	// stfs f11,0(r26)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r26.u32 + 0, temp.u32);
	// stfs f0,0(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// fsubs f13,f12,f13
	ctx.f13.f64 = double(float(ctx.f12.f64 - ctx.f13.f64));
	// fmadds f0,f8,f5,f30
	ctx.f0.f64 = double(float(std::fma(ctx.f8.f64, ctx.f5.f64, ctx.f30.f64)));
	// stfs f13,4(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 4, temp.u32);
	// fmadds f12,f9,f3,f2
	ctx.f12.f64 = double(float(std::fma(ctx.f9.f64, ctx.f3.f64, ctx.f2.f64)));
	// addi r31,r31,-8
	ctx.r31.s64 = ctx.r31.s64 + -8;
	// fmsubs f11,f10,f3,f4
	ctx.f11.f64 = double(float(std::fma(ctx.f10.f64, ctx.f3.f64, -ctx.f4.f64)));
	// fmsubs f13,f7,f5,f6
	ctx.f13.f64 = double(float(std::fma(ctx.f7.f64, ctx.f5.f64, -ctx.f6.f64)));
	// fadds f10,f12,f0
	ctx.f10.f64 = double(float(ctx.f12.f64 + ctx.f0.f64));
	// stfs f10,0(r30)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// fsubs f0,f0,f12
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f12.f64));
	// fadds f10,f11,f13
	ctx.f10.f64 = double(float(ctx.f11.f64 + ctx.f13.f64));
	// stfs f10,4(r30)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r30.u32 + 4, temp.u32);
	// stfs f0,0(r28)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r28.u32 + 0, temp.u32);
	// fsubs f0,f13,f11
	ctx.f0.f64 = double(float(ctx.f13.f64 - ctx.f11.f64));
	// stfs f0,0(r25)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r25.u32 + 0, temp.u32);
	// addi r28,r28,-8
	ctx.r28.s64 = ctx.r28.s64 + -8;
	// addi r30,r30,-8
	ctx.r30.s64 = ctx.r30.s64 + -8;
	// bne cr6,0x8295c08c
	if (!ctx.cr6.eq) goto loc_8295C08C;
loc_8295C270:
	// add r8,r23,r5
	ctx.r8.u64 = ctx.r23.u64 + ctx.r5.u64;
	// add r9,r11,r24
	ctx.r9.u64 = ctx.r11.u64 + ctx.r24.u64;
	// rlwinm r10,r24,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 2) & 0xFFFFFFFC;
	// add r7,r9,r11
	ctx.r7.u64 = ctx.r9.u64 + ctx.r11.u64;
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// lfs f0,0(r8)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// rlwinm r9,r9,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f13,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// add r8,r7,r11
	ctx.r8.u64 = ctx.r7.u64 + ctx.r11.u64;
	// rlwinm r11,r7,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r8,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// add r11,r11,r4
	ctx.r11.u64 = ctx.r11.u64 + ctx.r4.u64;
	// lfs f12,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// lfs f10,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f10.f64 = double(temp.f32);
	// add r8,r8,r4
	ctx.r8.u64 = ctx.r8.u64 + ctx.r4.u64;
	// lfs f6,4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4);
	ctx.f6.f64 = double(temp.f32);
	// fsubs f3,f12,f6
	ctx.f3.f64 = double(float(ctx.f12.f64 - ctx.f6.f64));
	// lfs f9,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f9.f64 = double(temp.f32);
	// fadds f5,f9,f10
	ctx.f5.f64 = double(float(ctx.f9.f64 + ctx.f10.f64));
	// lfs f7,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f7.f64 = double(temp.f32);
	// lfs f8,4(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 4);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f10,f10,f9
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f9.f64));
	// lfs f9,4(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 4);
	ctx.f9.f64 = double(temp.f32);
	// fadds f4,f8,f7
	ctx.f4.f64 = double(float(ctx.f8.f64 + ctx.f7.f64));
	// lfs f11,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fadds f12,f6,f12
	ctx.f12.f64 = double(float(ctx.f6.f64 + ctx.f12.f64));
	// fsubs f8,f8,f7
	ctx.f8.f64 = double(float(ctx.f8.f64 - ctx.f7.f64));
	// fsubs f7,f11,f9
	ctx.f7.f64 = double(float(ctx.f11.f64 - ctx.f9.f64));
	// fadds f11,f9,f11
	ctx.f11.f64 = double(float(ctx.f9.f64 + ctx.f11.f64));
	// fmuls f6,f13,f3
	ctx.f6.f64 = double(float(ctx.f13.f64 * ctx.f3.f64));
	// fmuls f2,f13,f5
	ctx.f2.f64 = double(float(ctx.f13.f64 * ctx.f5.f64));
	// fmuls f1,f0,f10
	ctx.f1.f64 = double(float(ctx.f0.f64 * ctx.f10.f64));
	// fmuls f31,f0,f4
	ctx.f31.f64 = double(float(ctx.f0.f64 * ctx.f4.f64));
	// fmuls f30,f0,f12
	ctx.f30.f64 = double(float(ctx.f0.f64 * ctx.f12.f64));
	// fmuls f29,f13,f8
	ctx.f29.f64 = double(float(ctx.f13.f64 * ctx.f8.f64));
	// fmadds f6,f0,f5,f6
	ctx.f6.f64 = double(float(std::fma(ctx.f0.f64, ctx.f5.f64, ctx.f6.f64)));
	// fmuls f5,f0,f7
	ctx.f5.f64 = double(float(ctx.f0.f64 * ctx.f7.f64));
	// fmsubs f9,f0,f3,f2
	ctx.f9.f64 = double(float(std::fma(ctx.f0.f64, ctx.f3.f64, -ctx.f2.f64)));
	// fmuls f3,f13,f11
	ctx.f3.f64 = double(float(ctx.f13.f64 * ctx.f11.f64));
	// fmsubs f7,f13,f7,f31
	ctx.f7.f64 = double(float(std::fma(ctx.f13.f64, ctx.f7.f64, -ctx.f31.f64)));
	// fmsubs f11,f0,f11,f29
	ctx.f11.f64 = double(float(std::fma(ctx.f0.f64, ctx.f11.f64, -ctx.f29.f64)));
	// fmsubs f12,f13,f12,f1
	ctx.f12.f64 = double(float(std::fma(ctx.f13.f64, ctx.f12.f64, -ctx.f1.f64)));
	// fmadds f5,f13,f4,f5
	ctx.f5.f64 = double(float(std::fma(ctx.f13.f64, ctx.f4.f64, ctx.f5.f64)));
	// fmadds f13,f13,f10,f30
	ctx.f13.f64 = double(float(std::fma(ctx.f13.f64, ctx.f10.f64, ctx.f30.f64)));
	// fmadds f0,f0,f8,f3
	ctx.f0.f64 = double(float(std::fma(ctx.f0.f64, ctx.f8.f64, ctx.f3.f64)));
	// fadds f4,f7,f9
	ctx.f4.f64 = double(float(ctx.f7.f64 + ctx.f9.f64));
	// stfs f4,0(r10)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// fsubs f9,f9,f7
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f7.f64));
	// fsubs f10,f12,f11
	ctx.f10.f64 = double(float(ctx.f12.f64 - ctx.f11.f64));
	// fadds f12,f11,f12
	ctx.f12.f64 = double(float(ctx.f11.f64 + ctx.f12.f64));
	// fadds f4,f5,f6
	ctx.f4.f64 = double(float(ctx.f5.f64 + ctx.f6.f64));
	// stfs f4,4(r10)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// stfs f9,0(r9)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r9.u32 + 0, temp.u32);
	// fsubs f9,f6,f5
	ctx.f9.f64 = double(float(ctx.f6.f64 - ctx.f5.f64));
	// stfs f9,4(r9)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r9.u32 + 4, temp.u32);
	// stfs f10,0(r11)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// fsubs f10,f13,f0
	ctx.f10.f64 = double(float(ctx.f13.f64 - ctx.f0.f64));
	// stfs f10,4(r11)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r11.u32 + 4, temp.u32);
	// fadds f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f13.f64));
	// stfs f12,0(r8)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r8.u32 + 0, temp.u32);
	// stfs f0,4(r8)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r8.u32 + 4, temp.u32);
	// addi r12,r1,-80
	ctx.r12.s64 = ctx.r1.s64 + -80;
	// bl 0x82414508
	ctx.lr = 0x8295C370;
	__restfpr_25(ctx, base);
	// b 0x824131e4
	__restgprlr_23(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295C378) {
	REX_FUNC_PROLOGUE(sub_8295C378);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// addi r12,r1,-8
	ctx.r12.s64 = ctx.r1.s64 + -8;
	// bl 0x82414490
	ctx.lr = 0x8295C388;
	__savefpr_14(ctx, base);
	// lfs f8,4(r3)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 4);
	ctx.f8.f64 = double(temp.f32);
	// lfs f9,68(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 68);
	ctx.f9.f64 = double(temp.f32);
	// lfs f11,8(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 8);
	ctx.f11.f64 = double(temp.f32);
	// fadds f25,f8,f9
	ctx.f25.f64 = double(float(ctx.f8.f64 + ctx.f9.f64));
	// lfs f0,4(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 4);
	ctx.f0.f64 = double(temp.f32);
	// fsubs f9,f8,f9
	ctx.f9.f64 = double(float(ctx.f8.f64 - ctx.f9.f64));
	// lfs f12,0(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// fmuls f13,f11,f0
	ctx.f13.f64 = double(float(ctx.f11.f64 * ctx.f0.f64));
	// lfs f6,32(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 32);
	ctx.f6.f64 = double(temp.f32);
	// lfs f10,64(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 64);
	ctx.f10.f64 = double(temp.f32);
	// lfs f7,96(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 96);
	ctx.f7.f64 = double(temp.f32);
	// fadds f26,f12,f10
	ctx.f26.f64 = double(float(ctx.f12.f64 + ctx.f10.f64));
	// lfs f4,36(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 36);
	ctx.f4.f64 = double(temp.f32);
	// fadds f8,f6,f7
	ctx.f8.f64 = double(float(ctx.f6.f64 + ctx.f7.f64));
	// lfs f5,100(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 100);
	ctx.f5.f64 = double(temp.f32);
	// fsubs f7,f6,f7
	ctx.f7.f64 = double(float(ctx.f6.f64 - ctx.f7.f64));
	// fadds f6,f4,f5
	ctx.f6.f64 = double(float(ctx.f4.f64 + ctx.f5.f64));
	// lfs f2,8(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 8);
	ctx.f2.f64 = double(temp.f32);
	// fsubs f10,f12,f10
	ctx.f10.f64 = double(float(ctx.f12.f64 - ctx.f10.f64));
	// lfs f3,72(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 72);
	ctx.f3.f64 = double(temp.f32);
	// fsubs f5,f4,f5
	ctx.f5.f64 = double(float(ctx.f4.f64 - ctx.f5.f64));
	// lfs f1,76(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 76);
	ctx.f1.f64 = double(temp.f32);
	// lfs f31,12(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 12);
	ctx.f31.f64 = double(temp.f32);
	// fadds f12,f11,f13
	ctx.f12.f64 = double(float(ctx.f11.f64 + ctx.f13.f64));
	// lfs f28,108(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 108);
	ctx.f28.f64 = double(temp.f32);
	// lfs f27,44(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 44);
	ctx.f27.f64 = double(temp.f32);
	// lfs f30,104(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 104);
	ctx.f30.f64 = double(temp.f32);
	// lfs f29,40(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 40);
	ctx.f29.f64 = double(temp.f32);
	// fadds f11,f8,f26
	ctx.f11.f64 = double(float(ctx.f8.f64 + ctx.f26.f64));
	// lfs f22,112(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 112);
	ctx.f22.f64 = double(temp.f32);
	// fsubs f8,f26,f8
	ctx.f8.f64 = double(float(ctx.f26.f64 - ctx.f8.f64));
	// lfs f21,48(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 48);
	ctx.f21.f64 = double(temp.f32);
	// fadds f26,f2,f3
	ctx.f26.f64 = double(float(ctx.f2.f64 + ctx.f3.f64));
	// lfs f20,116(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 116);
	ctx.f20.f64 = double(temp.f32);
	// fadds f4,f6,f25
	ctx.f4.f64 = double(float(ctx.f6.f64 + ctx.f25.f64));
	// lfs f19,52(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 52);
	ctx.f19.f64 = double(temp.f32);
	// fsubs f3,f2,f3
	ctx.f3.f64 = double(float(ctx.f2.f64 - ctx.f3.f64));
	// lfs f18,88(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 88);
	ctx.f18.f64 = double(temp.f32);
	// fsubs f6,f25,f6
	ctx.f6.f64 = double(float(ctx.f25.f64 - ctx.f6.f64));
	// lfs f17,24(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 24);
	ctx.f17.f64 = double(temp.f32);
	// fadds f25,f31,f1
	ctx.f25.f64 = double(float(ctx.f31.f64 + ctx.f1.f64));
	// lfs f16,92(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 92);
	ctx.f16.f64 = double(temp.f32);
	// fsubs f2,f31,f1
	ctx.f2.f64 = double(float(ctx.f31.f64 - ctx.f1.f64));
	// lfs f15,28(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 28);
	ctx.f15.f64 = double(temp.f32);
	// fsubs f31,f27,f28
	ctx.f31.f64 = double(float(ctx.f27.f64 - ctx.f28.f64));
	// fsubs f24,f10,f5
	ctx.f24.f64 = double(float(ctx.f10.f64 - ctx.f5.f64));
	// fadds f10,f5,f10
	ctx.f10.f64 = double(float(ctx.f5.f64 + ctx.f10.f64));
	// fadds f5,f27,f28
	ctx.f5.f64 = double(float(ctx.f27.f64 + ctx.f28.f64));
	// fsubs f1,f29,f30
	ctx.f1.f64 = double(float(ctx.f29.f64 - ctx.f30.f64));
	// fadds f23,f7,f9
	ctx.f23.f64 = double(float(ctx.f7.f64 + ctx.f9.f64));
	// fsubs f9,f9,f7
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f7.f64));
	// fadds f7,f29,f30
	ctx.f7.f64 = double(float(ctx.f29.f64 + ctx.f30.f64));
	// fsubs f27,f3,f31
	ctx.f27.f64 = double(float(ctx.f3.f64 - ctx.f31.f64));
	// fadds f3,f31,f3
	ctx.f3.f64 = double(float(ctx.f31.f64 + ctx.f3.f64));
	// fadds f29,f5,f25
	ctx.f29.f64 = double(float(ctx.f5.f64 + ctx.f25.f64));
	// fsubs f5,f25,f5
	ctx.f5.f64 = double(float(ctx.f25.f64 - ctx.f5.f64));
	// fadds f28,f1,f2
	ctx.f28.f64 = double(float(ctx.f1.f64 + ctx.f2.f64));
	// fsubs f2,f2,f1
	ctx.f2.f64 = double(float(ctx.f2.f64 - ctx.f1.f64));
	// fadds f30,f7,f26
	ctx.f30.f64 = double(float(ctx.f7.f64 + ctx.f26.f64));
	// fsubs f7,f26,f7
	ctx.f7.f64 = double(float(ctx.f26.f64 - ctx.f7.f64));
	// fmuls f25,f27,f13
	ctx.f25.f64 = double(float(ctx.f27.f64 * ctx.f13.f64));
	// fmuls f26,f28,f13
	ctx.f26.f64 = double(float(ctx.f28.f64 * ctx.f13.f64));
	// fmadds f31,f28,f12,f25
	ctx.f31.f64 = double(float(std::fma(ctx.f28.f64, ctx.f12.f64, ctx.f25.f64)));
	// lfs f25,20(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 20);
	ctx.f25.f64 = double(temp.f32);
	// fmuls f28,f2,f12
	ctx.f28.f64 = double(float(ctx.f2.f64 * ctx.f12.f64));
	// fmuls f2,f2,f13
	ctx.f2.f64 = double(float(ctx.f2.f64 * ctx.f13.f64));
	// fmsubs f1,f27,f12,f26
	ctx.f1.f64 = double(float(std::fma(ctx.f27.f64, ctx.f12.f64, -ctx.f26.f64)));
	// lfs f27,16(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 16);
	ctx.f27.f64 = double(temp.f32);
	// lfs f26,84(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 84);
	ctx.f26.f64 = double(temp.f32);
	// fmsubs f28,f3,f13,f28
	ctx.f28.f64 = double(float(std::fma(ctx.f3.f64, ctx.f13.f64, -ctx.f28.f64)));
	// fmadds f3,f3,f12,f2
	ctx.f3.f64 = double(float(std::fma(ctx.f3.f64, ctx.f12.f64, ctx.f2.f64)));
	// lfs f2,80(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 80);
	ctx.f2.f64 = double(temp.f32);
	// fadds f14,f27,f2
	ctx.f14.f64 = double(float(ctx.f27.f64 + ctx.f2.f64));
	// fsubs f27,f27,f2
	ctx.f27.f64 = double(float(ctx.f27.f64 - ctx.f2.f64));
	// stfs f27,-188(r1)
	temp.f32 = float(ctx.f27.f64);
	REX_STORE_U32(ctx.r1.u32 + -188, temp.u32);
	// fadds f2,f25,f26
	ctx.f2.f64 = double(float(ctx.f25.f64 + ctx.f26.f64));
	// fsubs f26,f25,f26
	ctx.f26.f64 = double(float(ctx.f25.f64 - ctx.f26.f64));
	// stfs f26,-192(r1)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r1.u32 + -192, temp.u32);
	// fadds f26,f21,f22
	ctx.f26.f64 = double(float(ctx.f21.f64 + ctx.f22.f64));
	// fsubs f22,f21,f22
	ctx.f22.f64 = double(float(ctx.f21.f64 - ctx.f22.f64));
	// fadds f25,f19,f20
	ctx.f25.f64 = double(float(ctx.f19.f64 + ctx.f20.f64));
	// fsubs f21,f19,f20
	ctx.f21.f64 = double(float(ctx.f19.f64 - ctx.f20.f64));
	// fadds f20,f26,f14
	ctx.f20.f64 = double(float(ctx.f26.f64 + ctx.f14.f64));
	// stfs f20,-168(r1)
	temp.f32 = float(ctx.f20.f64);
	REX_STORE_U32(ctx.r1.u32 + -168, temp.u32);
	// fsubs f26,f14,f26
	ctx.f26.f64 = double(float(ctx.f14.f64 - ctx.f26.f64));
	// stfs f26,-172(r1)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r1.u32 + -172, temp.u32);
	// fadds f26,f25,f2
	ctx.f26.f64 = double(float(ctx.f25.f64 + ctx.f2.f64));
	// stfs f26,-164(r1)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r1.u32 + -164, temp.u32);
	// fsubs f2,f2,f25
	ctx.f2.f64 = double(float(ctx.f2.f64 - ctx.f25.f64));
	// stfs f2,-176(r1)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r1.u32 + -176, temp.u32);
	// lfs f2,-192(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -192);
	ctx.f2.f64 = double(temp.f32);
	// fadds f19,f22,f2
	ctx.f19.f64 = double(float(ctx.f22.f64 + ctx.f2.f64));
	// lfs f2,120(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 120);
	ctx.f2.f64 = double(temp.f32);
	// lfs f25,60(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 60);
	ctx.f25.f64 = double(temp.f32);
	// fsubs f20,f27,f21
	ctx.f20.f64 = double(float(ctx.f27.f64 - ctx.f21.f64));
	// lfs f27,56(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 56);
	ctx.f27.f64 = double(temp.f32);
	// fadds f26,f27,f2
	ctx.f26.f64 = double(float(ctx.f27.f64 + ctx.f2.f64));
	// stfs f26,-184(r1)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r1.u32 + -184, temp.u32);
	// lfs f26,124(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 124);
	ctx.f26.f64 = double(temp.f32);
	// fsubs f2,f27,f2
	ctx.f2.f64 = double(float(ctx.f27.f64 - ctx.f2.f64));
	// fadds f14,f25,f26
	ctx.f14.f64 = double(float(ctx.f25.f64 + ctx.f26.f64));
	// stfs f14,-180(r1)
	temp.f32 = float(ctx.f14.f64);
	REX_STORE_U32(ctx.r1.u32 + -180, temp.u32);
	// fsubs f26,f25,f26
	ctx.f26.f64 = double(float(ctx.f25.f64 - ctx.f26.f64));
	// fsubs f25,f15,f16
	ctx.f25.f64 = double(float(ctx.f15.f64 - ctx.f16.f64));
	// fsubs f14,f20,f19
	ctx.f14.f64 = double(float(ctx.f20.f64 - ctx.f19.f64));
	// fadds f19,f19,f20
	ctx.f19.f64 = double(float(ctx.f19.f64 + ctx.f20.f64));
	// lfs f20,-188(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -188);
	ctx.f20.f64 = double(temp.f32);
	// fadds f21,f21,f20
	ctx.f21.f64 = double(float(ctx.f21.f64 + ctx.f20.f64));
	// lfs f20,-192(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -192);
	ctx.f20.f64 = double(temp.f32);
	// fsubs f22,f20,f22
	ctx.f22.f64 = double(float(ctx.f20.f64 - ctx.f22.f64));
	// fsubs f20,f17,f18
	ctx.f20.f64 = double(float(ctx.f17.f64 - ctx.f18.f64));
	// fmuls f27,f14,f0
	ctx.f27.f64 = double(float(ctx.f14.f64 * ctx.f0.f64));
	// fmuls f19,f19,f0
	ctx.f19.f64 = double(float(ctx.f19.f64 * ctx.f0.f64));
	// fadds f14,f22,f21
	ctx.f14.f64 = double(float(ctx.f22.f64 + ctx.f21.f64));
	// fsubs f22,f22,f21
	ctx.f22.f64 = double(float(ctx.f22.f64 - ctx.f21.f64));
	// stfs f22,-188(r1)
	temp.f32 = float(ctx.f22.f64);
	REX_STORE_U32(ctx.r1.u32 + -188, temp.u32);
	// fadds f22,f17,f18
	ctx.f22.f64 = double(float(ctx.f17.f64 + ctx.f18.f64));
	// fadds f21,f15,f16
	ctx.f21.f64 = double(float(ctx.f15.f64 + ctx.f16.f64));
	// fmuls f18,f14,f0
	ctx.f18.f64 = double(float(ctx.f14.f64 * ctx.f0.f64));
	// lfs f15,-184(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -184);
	ctx.f15.f64 = double(temp.f32);
	// fadds f16,f15,f22
	ctx.f16.f64 = double(float(ctx.f15.f64 + ctx.f22.f64));
	// fsubs f22,f22,f15
	ctx.f22.f64 = double(float(ctx.f22.f64 - ctx.f15.f64));
	// stfs f22,-184(r1)
	temp.f32 = float(ctx.f22.f64);
	REX_STORE_U32(ctx.r1.u32 + -184, temp.u32);
	// lfs f22,-180(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -180);
	ctx.f22.f64 = double(temp.f32);
	// fadds f15,f22,f21
	ctx.f15.f64 = double(float(ctx.f22.f64 + ctx.f21.f64));
	// fsubs f22,f21,f22
	ctx.f22.f64 = double(float(ctx.f21.f64 - ctx.f22.f64));
	// stfs f22,-180(r1)
	temp.f32 = float(ctx.f22.f64);
	REX_STORE_U32(ctx.r1.u32 + -180, temp.u32);
	// fadds f21,f2,f25
	ctx.f21.f64 = double(float(ctx.f2.f64 + ctx.f25.f64));
	// fsubs f22,f20,f26
	ctx.f22.f64 = double(float(ctx.f20.f64 - ctx.f26.f64));
	// fadds f26,f26,f20
	ctx.f26.f64 = double(float(ctx.f26.f64 + ctx.f20.f64));
	// fsubs f2,f25,f2
	ctx.f2.f64 = double(float(ctx.f25.f64 - ctx.f2.f64));
	// fmr f20,f26
	ctx.f20.f64 = ctx.f26.f64;
	// lfs f14,-188(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -188);
	ctx.f14.f64 = double(temp.f32);
	// fmuls f17,f14,f0
	ctx.f17.f64 = double(float(ctx.f14.f64 * ctx.f0.f64));
	// stfs f15,-188(r1)
	temp.f32 = float(ctx.f15.f64);
	REX_STORE_U32(ctx.r1.u32 + -188, temp.u32);
	// fmuls f14,f21,f12
	ctx.f14.f64 = double(float(ctx.f21.f64 * ctx.f12.f64));
	// fmuls f21,f21,f13
	ctx.f21.f64 = double(float(ctx.f21.f64 * ctx.f13.f64));
	// fmr f15,f22
	ctx.f15.f64 = ctx.f22.f64;
	// fadds f25,f17,f9
	ctx.f25.f64 = double(float(ctx.f17.f64 + ctx.f9.f64));
	// fmsubs f22,f22,f13,f14
	ctx.f22.f64 = double(float(std::fma(ctx.f22.f64, ctx.f13.f64, -ctx.f14.f64)));
	// fmadds f21,f15,f12,f21
	ctx.f21.f64 = double(float(std::fma(ctx.f15.f64, ctx.f12.f64, ctx.f21.f64)));
	// fmuls f15,f26,f13
	ctx.f15.f64 = double(float(ctx.f26.f64 * ctx.f13.f64));
	// fsubs f26,f10,f18
	ctx.f26.f64 = double(float(ctx.f10.f64 - ctx.f18.f64));
	// fadds f10,f18,f10
	ctx.f10.f64 = double(float(ctx.f18.f64 + ctx.f10.f64));
	// fmuls f18,f2,f13
	ctx.f18.f64 = double(float(ctx.f2.f64 * ctx.f13.f64));
	// fsubs f13,f9,f17
	ctx.f13.f64 = double(float(ctx.f9.f64 - ctx.f17.f64));
	// fmsubs f9,f20,f12,f18
	ctx.f9.f64 = double(float(std::fma(ctx.f20.f64, ctx.f12.f64, -ctx.f18.f64)));
	// fmadds f12,f2,f12,f15
	ctx.f12.f64 = double(float(std::fma(ctx.f2.f64, ctx.f12.f64, ctx.f15.f64)));
	// fsubs f2,f28,f9
	ctx.f2.f64 = double(float(ctx.f28.f64 - ctx.f9.f64));
	// fsubs f20,f3,f12
	ctx.f20.f64 = double(float(ctx.f3.f64 - ctx.f12.f64));
	// fadds f12,f12,f3
	ctx.f12.f64 = double(float(ctx.f12.f64 + ctx.f3.f64));
	// fadds f9,f9,f28
	ctx.f9.f64 = double(float(ctx.f9.f64 + ctx.f28.f64));
	// fadds f3,f2,f26
	ctx.f3.f64 = double(float(ctx.f2.f64 + ctx.f26.f64));
	// stfs f3,96(r3)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r3.u32 + 96, temp.u32);
	// fadds f3,f20,f13
	ctx.f3.f64 = double(float(ctx.f20.f64 + ctx.f13.f64));
	// stfs f3,100(r3)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r3.u32 + 100, temp.u32);
	// fsubs f3,f26,f2
	ctx.f3.f64 = double(float(ctx.f26.f64 - ctx.f2.f64));
	// stfs f3,104(r3)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r3.u32 + 104, temp.u32);
	// fsubs f13,f13,f20
	ctx.f13.f64 = double(float(ctx.f13.f64 - ctx.f20.f64));
	// stfs f13,108(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 108, temp.u32);
	// fadds f13,f27,f24
	ctx.f13.f64 = double(float(ctx.f27.f64 + ctx.f24.f64));
	// fadds f3,f19,f23
	ctx.f3.f64 = double(float(ctx.f19.f64 + ctx.f23.f64));
	// fadds f2,f22,f1
	ctx.f2.f64 = double(float(ctx.f22.f64 + ctx.f1.f64));
	// fadds f28,f21,f31
	ctx.f28.f64 = double(float(ctx.f21.f64 + ctx.f31.f64));
	// fsubs f26,f10,f12
	ctx.f26.f64 = double(float(ctx.f10.f64 - ctx.f12.f64));
	// stfs f26,112(r3)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r3.u32 + 112, temp.u32);
	// fadds f12,f12,f10
	ctx.f12.f64 = double(float(ctx.f12.f64 + ctx.f10.f64));
	// stfs f12,120(r3)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r3.u32 + 120, temp.u32);
	// fsubs f12,f25,f9
	ctx.f12.f64 = double(float(ctx.f25.f64 - ctx.f9.f64));
	// stfs f12,124(r3)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r3.u32 + 124, temp.u32);
	// fadds f26,f9,f25
	ctx.f26.f64 = double(float(ctx.f9.f64 + ctx.f25.f64));
	// stfs f26,116(r3)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r3.u32 + 116, temp.u32);
	// fsubs f9,f1,f22
	ctx.f9.f64 = double(float(ctx.f1.f64 - ctx.f22.f64));
	// fsubs f1,f31,f21
	ctx.f1.f64 = double(float(ctx.f31.f64 - ctx.f21.f64));
	// fsubs f10,f23,f19
	ctx.f10.f64 = double(float(ctx.f23.f64 - ctx.f19.f64));
	// fsubs f12,f24,f27
	ctx.f12.f64 = double(float(ctx.f24.f64 - ctx.f27.f64));
	// fadds f31,f2,f13
	ctx.f31.f64 = double(float(ctx.f2.f64 + ctx.f13.f64));
	// stfs f31,64(r3)
	temp.f32 = float(ctx.f31.f64);
	REX_STORE_U32(ctx.r3.u32 + 64, temp.u32);
	// fsubs f13,f13,f2
	ctx.f13.f64 = double(float(ctx.f13.f64 - ctx.f2.f64));
	// stfs f13,72(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 72, temp.u32);
	// fadds f31,f28,f3
	ctx.f31.f64 = double(float(ctx.f28.f64 + ctx.f3.f64));
	// stfs f31,68(r3)
	temp.f32 = float(ctx.f31.f64);
	REX_STORE_U32(ctx.r3.u32 + 68, temp.u32);
	// fsubs f13,f3,f28
	ctx.f13.f64 = double(float(ctx.f3.f64 - ctx.f28.f64));
	// lfs f31,-184(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -184);
	ctx.f31.f64 = double(temp.f32);
	// lfs f28,-180(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -180);
	ctx.f28.f64 = double(temp.f32);
	// fadds f3,f31,f5
	ctx.f3.f64 = double(float(ctx.f31.f64 + ctx.f5.f64));
	// stfs f13,76(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 76, temp.u32);
	// fsubs f13,f7,f28
	ctx.f13.f64 = double(float(ctx.f7.f64 - ctx.f28.f64));
	// fadds f2,f9,f10
	ctx.f2.f64 = double(float(ctx.f9.f64 + ctx.f10.f64));
	// stfs f2,84(r3)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r3.u32 + 84, temp.u32);
	// fsubs f2,f12,f1
	ctx.f2.f64 = double(float(ctx.f12.f64 - ctx.f1.f64));
	// stfs f2,80(r3)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r3.u32 + 80, temp.u32);
	// fadds f12,f1,f12
	ctx.f12.f64 = double(float(ctx.f1.f64 + ctx.f12.f64));
	// stfs f12,88(r3)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r3.u32 + 88, temp.u32);
	// fsubs f12,f10,f9
	ctx.f12.f64 = double(float(ctx.f10.f64 - ctx.f9.f64));
	// lfs f1,-176(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -176);
	ctx.f1.f64 = double(temp.f32);
	// fsubs f9,f5,f31
	ctx.f9.f64 = double(float(ctx.f5.f64 - ctx.f31.f64));
	// lfs f2,-172(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -172);
	ctx.f2.f64 = double(temp.f32);
	// stfs f12,92(r3)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r3.u32 + 92, temp.u32);
	// fadds f12,f1,f8
	ctx.f12.f64 = double(float(ctx.f1.f64 + ctx.f8.f64));
	// fsubs f10,f6,f2
	ctx.f10.f64 = double(float(ctx.f6.f64 - ctx.f2.f64));
	// fsubs f27,f13,f3
	ctx.f27.f64 = double(float(ctx.f13.f64 - ctx.f3.f64));
	// fadds f3,f3,f13
	ctx.f3.f64 = double(float(ctx.f3.f64 + ctx.f13.f64));
	// fadds f13,f28,f7
	ctx.f13.f64 = double(float(ctx.f28.f64 + ctx.f7.f64));
	// fmuls f7,f27,f0
	ctx.f7.f64 = double(float(ctx.f27.f64 * ctx.f0.f64));
	// fmuls f5,f3,f0
	ctx.f5.f64 = double(float(ctx.f3.f64 * ctx.f0.f64));
	// fsubs f3,f13,f9
	ctx.f3.f64 = double(float(ctx.f13.f64 - ctx.f9.f64));
	// fadds f31,f9,f13
	ctx.f31.f64 = double(float(ctx.f9.f64 + ctx.f13.f64));
	// fsubs f13,f8,f1
	ctx.f13.f64 = double(float(ctx.f8.f64 - ctx.f1.f64));
	// fadds f9,f2,f6
	ctx.f9.f64 = double(float(ctx.f2.f64 + ctx.f6.f64));
	// fmuls f8,f3,f0
	ctx.f8.f64 = double(float(ctx.f3.f64 * ctx.f0.f64));
	// fmuls f0,f31,f0
	ctx.f0.f64 = double(float(ctx.f31.f64 * ctx.f0.f64));
	// fadds f6,f7,f13
	ctx.f6.f64 = double(float(ctx.f7.f64 + ctx.f13.f64));
	// stfs f6,32(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 32, temp.u32);
	// fsubs f13,f13,f7
	ctx.f13.f64 = double(float(ctx.f13.f64 - ctx.f7.f64));
	// stfs f13,40(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 40, temp.u32);
	// fsubs f13,f9,f5
	ctx.f13.f64 = double(float(ctx.f9.f64 - ctx.f5.f64));
	// stfs f13,44(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 44, temp.u32);
	// lfs f7,-188(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -188);
	ctx.f7.f64 = double(temp.f32);
	// fadds f6,f5,f9
	ctx.f6.f64 = double(float(ctx.f5.f64 + ctx.f9.f64));
	// fadds f9,f7,f29
	ctx.f9.f64 = double(float(ctx.f7.f64 + ctx.f29.f64));
	// stfs f6,36(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 36, temp.u32);
	// fsubs f7,f29,f7
	ctx.f7.f64 = double(float(ctx.f29.f64 - ctx.f7.f64));
	// fsubs f13,f12,f0
	ctx.f13.f64 = double(float(ctx.f12.f64 - ctx.f0.f64));
	// stfs f13,48(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 48, temp.u32);
	// fadds f0,f0,f12
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f12.f64));
	// stfs f0,56(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 56, temp.u32);
	// lfs f12,-168(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -168);
	ctx.f12.f64 = double(temp.f32);
	// fsubs f0,f10,f8
	ctx.f0.f64 = double(float(ctx.f10.f64 - ctx.f8.f64));
	// fadds f13,f8,f10
	ctx.f13.f64 = double(float(ctx.f8.f64 + ctx.f10.f64));
	// stfs f0,60(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 60, temp.u32);
	// lfs f10,-164(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -164);
	ctx.f10.f64 = double(temp.f32);
	// fadds f0,f12,f11
	ctx.f0.f64 = double(float(ctx.f12.f64 + ctx.f11.f64));
	// stfs f13,52(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 52, temp.u32);
	// fsubs f12,f11,f12
	ctx.f12.f64 = double(float(ctx.f11.f64 - ctx.f12.f64));
	// fadds f13,f10,f4
	ctx.f13.f64 = double(float(ctx.f10.f64 + ctx.f4.f64));
	// fsubs f11,f4,f10
	ctx.f11.f64 = double(float(ctx.f4.f64 - ctx.f10.f64));
	// fadds f10,f16,f30
	ctx.f10.f64 = double(float(ctx.f16.f64 + ctx.f30.f64));
	// fsubs f8,f30,f16
	ctx.f8.f64 = double(float(ctx.f30.f64 - ctx.f16.f64));
	// fadds f6,f10,f0
	ctx.f6.f64 = double(float(ctx.f10.f64 + ctx.f0.f64));
	// stfs f6,0(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 0, temp.u32);
	// fadds f6,f9,f13
	ctx.f6.f64 = double(float(ctx.f9.f64 + ctx.f13.f64));
	// stfs f6,4(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 4, temp.u32);
	// fsubs f0,f0,f10
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f10.f64));
	// stfs f0,8(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 8, temp.u32);
	// fsubs f0,f13,f9
	ctx.f0.f64 = double(float(ctx.f13.f64 - ctx.f9.f64));
	// stfs f0,12(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 12, temp.u32);
	// fsubs f0,f12,f7
	ctx.f0.f64 = double(float(ctx.f12.f64 - ctx.f7.f64));
	// stfs f0,16(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 16, temp.u32);
	// fadds f0,f8,f11
	ctx.f0.f64 = double(float(ctx.f8.f64 + ctx.f11.f64));
	// stfs f0,20(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 20, temp.u32);
	// fadds f0,f7,f12
	ctx.f0.f64 = double(float(ctx.f7.f64 + ctx.f12.f64));
	// stfs f0,24(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 24, temp.u32);
	// fsubs f0,f11,f8
	ctx.f0.f64 = double(float(ctx.f11.f64 - ctx.f8.f64));
	// stfs f0,28(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 28, temp.u32);
	// addi r12,r1,-8
	ctx.r12.s64 = ctx.r1.s64 + -8;
	// bl 0x824144dc
	ctx.lr = 0x8295C78C;
	__restfpr_14(ctx, base);
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_8295C798) {
	REX_FUNC_PROLOGUE(sub_8295C798);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// addi r12,r1,-8
	ctx.r12.s64 = ctx.r1.s64 + -8;
	// bl 0x82414490
	ctx.lr = 0x8295C7A8;
	__savefpr_14(ctx, base);
	// lfs f5,100(r3)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 100);
	ctx.f5.f64 = double(temp.f32);
	// lfs f4,96(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 96);
	ctx.f4.f64 = double(temp.f32);
	// lfs f3,36(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 36);
	ctx.f3.f64 = double(temp.f32);
	// lfs f6,32(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 32);
	ctx.f6.f64 = double(temp.f32);
	// fadds f21,f3,f4
	ctx.f21.f64 = double(float(ctx.f3.f64 + ctx.f4.f64));
	// fsubs f22,f6,f5
	ctx.f22.f64 = double(float(ctx.f6.f64 - ctx.f5.f64));
	// lfs f0,4(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 4);
	ctx.f0.f64 = double(temp.f32);
	// fadds f6,f5,f6
	ctx.f6.f64 = double(float(ctx.f5.f64 + ctx.f6.f64));
	// lfs f28,64(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 64);
	ctx.f28.f64 = double(temp.f32);
	// fsubs f5,f3,f4
	ctx.f5.f64 = double(float(ctx.f3.f64 - ctx.f4.f64));
	// lfs f27,4(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 4);
	ctx.f27.f64 = double(temp.f32);
	// fadds f19,f27,f28
	ctx.f19.f64 = double(float(ctx.f27.f64 + ctx.f28.f64));
	// lfs f2,8(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 8);
	ctx.f2.f64 = double(temp.f32);
	// lfs f1,76(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 76);
	ctx.f1.f64 = double(temp.f32);
	// lfs f31,72(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 72);
	ctx.f31.f64 = double(temp.f32);
	// lfs f30,12(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 12);
	ctx.f30.f64 = double(temp.f32);
	// lfs f29,68(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 68);
	ctx.f29.f64 = double(temp.f32);
	// lfs f7,0(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 0);
	ctx.f7.f64 = double(temp.f32);
	// fsubs f20,f7,f29
	ctx.f20.f64 = double(float(ctx.f7.f64 - ctx.f29.f64));
	// lfs f12,20(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 20);
	ctx.f12.f64 = double(temp.f32);
	// fsubs f18,f22,f21
	ctx.f18.f64 = double(float(ctx.f22.f64 - ctx.f21.f64));
	// lfs f13,16(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 16);
	ctx.f13.f64 = double(temp.f32);
	// fadds f22,f21,f22
	ctx.f22.f64 = double(float(ctx.f21.f64 + ctx.f22.f64));
	// lfs f25,108(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 108);
	ctx.f25.f64 = double(temp.f32);
	// fadds f17,f5,f6
	ctx.f17.f64 = double(float(ctx.f5.f64 + ctx.f6.f64));
	// lfs f23,44(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 44);
	ctx.f23.f64 = double(temp.f32);
	// fadds f7,f29,f7
	ctx.f7.f64 = double(float(ctx.f29.f64 + ctx.f7.f64));
	// lfs f26,40(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 40);
	ctx.f26.f64 = double(temp.f32);
	// fsubs f29,f27,f28
	ctx.f29.f64 = double(float(ctx.f27.f64 - ctx.f28.f64));
	// lfs f24,104(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 104);
	ctx.f24.f64 = double(temp.f32);
	// lfs f11,24(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 24);
	ctx.f11.f64 = double(temp.f32);
	// lfs f10,28(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 28);
	ctx.f10.f64 = double(temp.f32);
	// lfs f9,32(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 32);
	ctx.f9.f64 = double(temp.f32);
	// lfs f8,36(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 36);
	ctx.f8.f64 = double(temp.f32);
	// fmuls f4,f18,f0
	ctx.f4.f64 = double(float(ctx.f18.f64 * ctx.f0.f64));
	// fsubs f18,f6,f5
	ctx.f18.f64 = double(float(ctx.f6.f64 - ctx.f5.f64));
	// fmuls f3,f22,f0
	ctx.f3.f64 = double(float(ctx.f22.f64 * ctx.f0.f64));
	// fsubs f6,f2,f1
	ctx.f6.f64 = double(float(ctx.f2.f64 - ctx.f1.f64));
	// fadds f5,f30,f31
	ctx.f5.f64 = double(float(ctx.f30.f64 + ctx.f31.f64));
	// fmuls f27,f17,f0
	ctx.f27.f64 = double(float(ctx.f17.f64 * ctx.f0.f64));
	// fadds f22,f4,f20
	ctx.f22.f64 = double(float(ctx.f4.f64 + ctx.f20.f64));
	// fmuls f28,f18,f0
	ctx.f28.f64 = double(float(ctx.f18.f64 * ctx.f0.f64));
	// fadds f21,f3,f19
	ctx.f21.f64 = double(float(ctx.f3.f64 + ctx.f19.f64));
	// fsubs f3,f19,f3
	ctx.f3.f64 = double(float(ctx.f19.f64 - ctx.f3.f64));
	// fmr f19,f6
	ctx.f19.f64 = ctx.f6.f64;
	// fmuls f18,f5,f12
	ctx.f18.f64 = double(float(ctx.f5.f64 * ctx.f12.f64));
	// fmr f17,f6
	ctx.f17.f64 = ctx.f6.f64;
	// fmuls f16,f5,f13
	ctx.f16.f64 = double(float(ctx.f5.f64 * ctx.f13.f64));
	// fadds f5,f23,f24
	ctx.f5.f64 = double(float(ctx.f23.f64 + ctx.f24.f64));
	// fsubs f6,f26,f25
	ctx.f6.f64 = double(float(ctx.f26.f64 - ctx.f25.f64));
	// fsubs f4,f20,f4
	ctx.f4.f64 = double(float(ctx.f20.f64 - ctx.f4.f64));
	// fadds f20,f28,f29
	ctx.f20.f64 = double(float(ctx.f28.f64 + ctx.f29.f64));
	// fsubs f29,f29,f28
	ctx.f29.f64 = double(float(ctx.f29.f64 - ctx.f28.f64));
	// fsubs f28,f7,f27
	ctx.f28.f64 = double(float(ctx.f7.f64 - ctx.f27.f64));
	// fadds f7,f27,f7
	ctx.f7.f64 = double(float(ctx.f27.f64 + ctx.f7.f64));
	// fmsubs f27,f19,f13,f18
	ctx.f27.f64 = double(float(std::fma(ctx.f19.f64, ctx.f13.f64, -ctx.f18.f64)));
	// fadds f26,f25,f26
	ctx.f26.f64 = double(float(ctx.f25.f64 + ctx.f26.f64));
	// lfs f25,48(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 48);
	ctx.f25.f64 = double(temp.f32);
	// fmadds f19,f17,f12,f16
	ctx.f19.f64 = double(float(std::fma(ctx.f17.f64, ctx.f12.f64, ctx.f16.f64)));
	// fmuls f18,f5,f11
	ctx.f18.f64 = double(float(ctx.f5.f64 * ctx.f11.f64));
	// fmr f17,f5
	ctx.f17.f64 = ctx.f5.f64;
	// fmuls f16,f6,f11
	ctx.f16.f64 = double(float(ctx.f6.f64 * ctx.f11.f64));
	// fadds f5,f1,f2
	ctx.f5.f64 = double(float(ctx.f1.f64 + ctx.f2.f64));
	// fsubs f2,f30,f31
	ctx.f2.f64 = double(float(ctx.f30.f64 - ctx.f31.f64));
	// lfs f30,16(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 16);
	ctx.f30.f64 = double(temp.f32);
	// fsubs f24,f23,f24
	ctx.f24.f64 = double(float(ctx.f23.f64 - ctx.f24.f64));
	// lfs f23,116(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 116);
	ctx.f23.f64 = double(temp.f32);
	// fmsubs f6,f6,f10,f18
	ctx.f6.f64 = double(float(std::fma(ctx.f6.f64, ctx.f10.f64, -ctx.f18.f64)));
	// fmadds f1,f17,f10,f16
	ctx.f1.f64 = double(float(std::fma(ctx.f17.f64, ctx.f10.f64, ctx.f16.f64)));
	// fmr f17,f5
	ctx.f17.f64 = ctx.f5.f64;
	// fmuls f16,f2,f10
	ctx.f16.f64 = double(float(ctx.f2.f64 * ctx.f10.f64));
	// fmr f15,f5
	ctx.f15.f64 = ctx.f5.f64;
	// lfs f5,80(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 80);
	ctx.f5.f64 = double(temp.f32);
	// fmuls f14,f2,f11
	ctx.f14.f64 = double(float(ctx.f2.f64 * ctx.f11.f64));
	// lfs f2,20(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 20);
	ctx.f2.f64 = double(temp.f32);
	// fadds f31,f6,f27
	ctx.f31.f64 = double(float(ctx.f6.f64 + ctx.f27.f64));
	// fsubs f6,f27,f6
	ctx.f6.f64 = double(float(ctx.f27.f64 - ctx.f6.f64));
	// lfs f27,84(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 84);
	ctx.f27.f64 = double(temp.f32);
	// fadds f18,f1,f19
	ctx.f18.f64 = double(float(ctx.f1.f64 + ctx.f19.f64));
	// stfs f18,-192(r1)
	temp.f32 = float(ctx.f18.f64);
	REX_STORE_U32(ctx.r1.u32 + -192, temp.u32);
	// lfs f18,52(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 52);
	ctx.f18.f64 = double(temp.f32);
	// fmsubs f17,f17,f11,f16
	ctx.f17.f64 = double(float(std::fma(ctx.f17.f64, ctx.f11.f64, -ctx.f16.f64)));
	// stfs f17,-204(r1)
	temp.f32 = float(ctx.f17.f64);
	REX_STORE_U32(ctx.r1.u32 + -204, temp.u32);
	// fsubs f19,f19,f1
	ctx.f19.f64 = double(float(ctx.f19.f64 - ctx.f1.f64));
	// fmadds f1,f15,f10,f14
	ctx.f1.f64 = double(float(std::fma(ctx.f15.f64, ctx.f10.f64, ctx.f14.f64)));
	// stfs f1,-208(r1)
	temp.f32 = float(ctx.f1.f64);
	REX_STORE_U32(ctx.r1.u32 + -208, temp.u32);
	// fmuls f16,f26,f13
	ctx.f16.f64 = double(float(ctx.f26.f64 * ctx.f13.f64));
	// fmr f15,f24
	ctx.f15.f64 = ctx.f24.f64;
	// fmuls f14,f26,f12
	ctx.f14.f64 = double(float(ctx.f26.f64 * ctx.f12.f64));
	// fsubs f1,f30,f27
	ctx.f1.f64 = double(float(ctx.f30.f64 - ctx.f27.f64));
	// fadds f26,f2,f5
	ctx.f26.f64 = double(float(ctx.f2.f64 + ctx.f5.f64));
	// fsubs f5,f2,f5
	ctx.f5.f64 = double(float(ctx.f2.f64 - ctx.f5.f64));
	// fadds f30,f27,f30
	ctx.f30.f64 = double(float(ctx.f27.f64 + ctx.f30.f64));
	// fmadds f24,f24,f12,f16
	ctx.f24.f64 = double(float(std::fma(ctx.f24.f64, ctx.f12.f64, ctx.f16.f64)));
	// fmsubs f16,f15,f13,f14
	ctx.f16.f64 = double(float(std::fma(ctx.f15.f64, ctx.f13.f64, -ctx.f14.f64)));
	// stfs f16,-200(r1)
	temp.f32 = float(ctx.f16.f64);
	REX_STORE_U32(ctx.r1.u32 + -200, temp.u32);
	// fmr f14,f1
	ctx.f14.f64 = ctx.f1.f64;
	// fmr f16,f1
	ctx.f16.f64 = ctx.f1.f64;
	// fmuls f1,f26,f9
	ctx.f1.f64 = double(float(ctx.f26.f64 * ctx.f9.f64));
	// stfs f1,-196(r1)
	temp.f32 = float(ctx.f1.f64);
	REX_STORE_U32(ctx.r1.u32 + -196, temp.u32);
	// fmuls f15,f26,f8
	ctx.f15.f64 = double(float(ctx.f26.f64 * ctx.f8.f64));
	// lfs f1,112(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 112);
	ctx.f1.f64 = double(temp.f32);
	// fsubs f26,f17,f24
	ctx.f26.f64 = double(float(ctx.f17.f64 - ctx.f24.f64));
	// stfs f26,-180(r1)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r1.u32 + -180, temp.u32);
	// fmr f26,f17
	ctx.f26.f64 = ctx.f17.f64;
	// fmsubs f16,f16,f9,f15
	ctx.f16.f64 = double(float(std::fma(ctx.f16.f64, ctx.f9.f64, -ctx.f15.f64)));
	// fadds f26,f24,f26
	ctx.f26.f64 = double(float(ctx.f24.f64 + ctx.f26.f64));
	// stfs f26,-188(r1)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r1.u32 + -188, temp.u32);
	// lfs f24,-208(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -208);
	ctx.f24.f64 = double(temp.f32);
	// lfs f26,-200(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -200);
	ctx.f26.f64 = double(temp.f32);
	// fsubs f24,f24,f26
	ctx.f24.f64 = double(float(ctx.f24.f64 - ctx.f26.f64));
	// stfs f24,-176(r1)
	temp.f32 = float(ctx.f24.f64);
	REX_STORE_U32(ctx.r1.u32 + -176, temp.u32);
	// lfs f24,-208(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -208);
	ctx.f24.f64 = double(temp.f32);
	// lfs f15,-196(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -196);
	ctx.f15.f64 = double(temp.f32);
	// fadds f17,f26,f24
	ctx.f17.f64 = double(float(ctx.f26.f64 + ctx.f24.f64));
	// fmadds f26,f14,f8,f15
	ctx.f26.f64 = double(float(std::fma(ctx.f14.f64, ctx.f8.f64, ctx.f15.f64)));
	// stfs f26,-208(r1)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r1.u32 + -208, temp.u32);
	// fsubs f26,f25,f23
	ctx.f26.f64 = double(float(ctx.f25.f64 - ctx.f23.f64));
	// fadds f24,f18,f1
	ctx.f24.f64 = double(float(ctx.f18.f64 + ctx.f1.f64));
	// fsubs f1,f18,f1
	ctx.f1.f64 = double(float(ctx.f18.f64 - ctx.f1.f64));
	// lfs f18,28(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 28);
	ctx.f18.f64 = double(temp.f32);
	// fmuls f14,f26,f9
	ctx.f14.f64 = double(float(ctx.f26.f64 * ctx.f9.f64));
	// fmuls f15,f24,f9
	ctx.f15.f64 = double(float(ctx.f24.f64 * ctx.f9.f64));
	// fmadds f27,f24,f8,f14
	ctx.f27.f64 = double(float(std::fma(ctx.f24.f64, ctx.f8.f64, ctx.f14.f64)));
	// fmsubs f2,f26,f8,f15
	ctx.f2.f64 = double(float(std::fma(ctx.f26.f64, ctx.f8.f64, -ctx.f15.f64)));
	// fmuls f15,f5,f9
	ctx.f15.f64 = double(float(ctx.f5.f64 * ctx.f9.f64));
	// fmr f14,f5
	ctx.f14.f64 = ctx.f5.f64;
	// fmuls f5,f30,f9
	ctx.f5.f64 = double(float(ctx.f30.f64 * ctx.f9.f64));
	// stfs f5,-196(r1)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r1.u32 + -196, temp.u32);
	// fadds f5,f23,f25
	ctx.f5.f64 = double(float(ctx.f23.f64 + ctx.f25.f64));
	// fmr f24,f30
	ctx.f24.f64 = ctx.f30.f64;
	// fmuls f23,f1,f8
	ctx.f23.f64 = double(float(ctx.f1.f64 * ctx.f8.f64));
	// fmuls f1,f1,f9
	ctx.f1.f64 = double(float(ctx.f1.f64 * ctx.f9.f64));
	// fadds f30,f2,f16
	ctx.f30.f64 = double(float(ctx.f2.f64 + ctx.f16.f64));
	// fsubs f2,f16,f2
	ctx.f2.f64 = double(float(ctx.f16.f64 - ctx.f2.f64));
	// lfs f16,56(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 56);
	ctx.f16.f64 = double(temp.f32);
	// fmsubs f9,f5,f9,f23
	ctx.f9.f64 = double(float(std::fma(ctx.f5.f64, ctx.f9.f64, -ctx.f23.f64)));
	// lfs f23,88(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 88);
	ctx.f23.f64 = double(temp.f32);
	// lfs f25,-208(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -208);
	ctx.f25.f64 = double(temp.f32);
	// fadds f26,f27,f25
	ctx.f26.f64 = double(float(ctx.f27.f64 + ctx.f25.f64));
	// fsubs f27,f25,f27
	ctx.f27.f64 = double(float(ctx.f25.f64 - ctx.f27.f64));
	// fmsubs f25,f24,f8,f15
	ctx.f25.f64 = double(float(std::fma(ctx.f24.f64, ctx.f8.f64, -ctx.f15.f64)));
	// lfs f15,120(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 120);
	ctx.f15.f64 = double(temp.f32);
	// lfs f24,-196(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -196);
	ctx.f24.f64 = double(temp.f32);
	// fmadds f24,f14,f8,f24
	ctx.f24.f64 = double(float(std::fma(ctx.f14.f64, ctx.f8.f64, ctx.f24.f64)));
	// fmadds f8,f5,f8,f1
	ctx.f8.f64 = double(float(std::fma(ctx.f5.f64, ctx.f8.f64, ctx.f1.f64)));
	// lfs f5,24(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 24);
	ctx.f5.f64 = double(temp.f32);
	// fsubs f14,f25,f9
	ctx.f14.f64 = double(float(ctx.f25.f64 - ctx.f9.f64));
	// lfs f1,92(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 92);
	ctx.f1.f64 = double(temp.f32);
	// fadds f9,f9,f25
	ctx.f9.f64 = double(float(ctx.f9.f64 + ctx.f25.f64));
	// stfs f9,-168(r1)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r1.u32 + -168, temp.u32);
	// stfs f14,-200(r1)
	temp.f32 = float(ctx.f14.f64);
	REX_STORE_U32(ctx.r1.u32 + -200, temp.u32);
	// fsubs f9,f24,f8
	ctx.f9.f64 = double(float(ctx.f24.f64 - ctx.f8.f64));
	// stfs f9,-184(r1)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r1.u32 + -184, temp.u32);
	// fadds f9,f8,f24
	ctx.f9.f64 = double(float(ctx.f8.f64 + ctx.f24.f64));
	// stfs f9,-172(r1)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r1.u32 + -172, temp.u32);
	// fsubs f9,f5,f1
	ctx.f9.f64 = double(float(ctx.f5.f64 - ctx.f1.f64));
	// fadds f8,f18,f23
	ctx.f8.f64 = double(float(ctx.f18.f64 + ctx.f23.f64));
	// fmr f24,f9
	ctx.f24.f64 = ctx.f9.f64;
	// fmr f14,f9
	ctx.f14.f64 = ctx.f9.f64;
	// fmuls f9,f8,f10
	ctx.f9.f64 = double(float(ctx.f8.f64 * ctx.f10.f64));
	// fmuls f8,f8,f11
	ctx.f8.f64 = double(float(ctx.f8.f64 * ctx.f11.f64));
	// stfs f9,-196(r1)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r1.u32 + -196, temp.u32);
	// lfs f9,60(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 60);
	ctx.f9.f64 = double(temp.f32);
	// fadds f25,f9,f15
	ctx.f25.f64 = double(float(ctx.f9.f64 + ctx.f15.f64));
	// lfs f9,-196(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -196);
	ctx.f9.f64 = double(temp.f32);
	// fmsubs f9,f24,f11,f9
	ctx.f9.f64 = double(float(std::fma(ctx.f24.f64, ctx.f11.f64, -ctx.f9.f64)));
	// stfs f9,-204(r1)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r1.u32 + -204, temp.u32);
	// fmadds f9,f14,f10,f8
	ctx.f9.f64 = double(float(std::fma(ctx.f14.f64, ctx.f10.f64, ctx.f8.f64)));
	// stfs f9,-208(r1)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r1.u32 + -208, temp.u32);
	// lfs f9,124(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 124);
	ctx.f9.f64 = double(temp.f32);
	// fmuls f14,f25,f13
	ctx.f14.f64 = double(float(ctx.f25.f64 * ctx.f13.f64));
	// fsubs f8,f16,f9
	ctx.f8.f64 = double(float(ctx.f16.f64 - ctx.f9.f64));
	// fmr f24,f8
	ctx.f24.f64 = ctx.f8.f64;
	// fmuls f8,f8,f13
	ctx.f8.f64 = double(float(ctx.f8.f64 * ctx.f13.f64));
	// stfs f8,-196(r1)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r1.u32 + -196, temp.u32);
	// fadds f8,f1,f5
	ctx.f8.f64 = double(float(ctx.f1.f64 + ctx.f5.f64));
	// fsubs f5,f18,f23
	ctx.f5.f64 = double(float(ctx.f18.f64 - ctx.f23.f64));
	// fmsubs f1,f24,f12,f14
	ctx.f1.f64 = double(float(std::fma(ctx.f24.f64, ctx.f12.f64, -ctx.f14.f64)));
	// fmr f23,f8
	ctx.f23.f64 = ctx.f8.f64;
	// fmuls f18,f5,f13
	ctx.f18.f64 = double(float(ctx.f5.f64 * ctx.f13.f64));
	// fmuls f13,f8,f13
	ctx.f13.f64 = double(float(ctx.f8.f64 * ctx.f13.f64));
	// fmr f14,f5
	ctx.f14.f64 = ctx.f5.f64;
	// lfs f5,-204(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -204);
	ctx.f5.f64 = double(temp.f32);
	// fadds f8,f1,f5
	ctx.f8.f64 = double(float(ctx.f1.f64 + ctx.f5.f64));
	// fsubs f5,f5,f1
	ctx.f5.f64 = double(float(ctx.f5.f64 - ctx.f1.f64));
	// lfs f24,-196(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -196);
	ctx.f24.f64 = double(temp.f32);
	// stfs f13,-196(r1)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r1.u32 + -196, temp.u32);
	// fmadds f25,f25,f12,f24
	ctx.f25.f64 = double(float(std::fma(ctx.f25.f64, ctx.f12.f64, ctx.f24.f64)));
	// fadds f13,f9,f16
	ctx.f13.f64 = double(float(ctx.f9.f64 + ctx.f16.f64));
	// lfs f9,60(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 60);
	ctx.f9.f64 = double(temp.f32);
	// fsubs f9,f9,f15
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f15.f64));
	// lfs f24,-208(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -208);
	ctx.f24.f64 = double(temp.f32);
	// fadds f1,f25,f24
	ctx.f1.f64 = double(float(ctx.f25.f64 + ctx.f24.f64));
	// fsubs f25,f24,f25
	ctx.f25.f64 = double(float(ctx.f24.f64 - ctx.f25.f64));
	// fmadds f24,f23,f12,f18
	ctx.f24.f64 = double(float(std::fma(ctx.f23.f64, ctx.f12.f64, ctx.f18.f64)));
	// fsubs f16,f6,f25
	ctx.f16.f64 = double(float(ctx.f6.f64 - ctx.f25.f64));
	// fadds f6,f25,f6
	ctx.f6.f64 = double(float(ctx.f25.f64 + ctx.f6.f64));
	// lfs f23,-196(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -196);
	ctx.f23.f64 = double(temp.f32);
	// fmsubs f12,f14,f12,f23
	ctx.f12.f64 = double(float(std::fma(ctx.f14.f64, ctx.f12.f64, -ctx.f23.f64)));
	// lfs f14,-192(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -192);
	ctx.f14.f64 = double(temp.f32);
	// fmuls f23,f9,f11
	ctx.f23.f64 = double(float(ctx.f9.f64 * ctx.f11.f64));
	// fmuls f11,f13,f11
	ctx.f11.f64 = double(float(ctx.f13.f64 * ctx.f11.f64));
	// fadds f18,f1,f14
	ctx.f18.f64 = double(float(ctx.f1.f64 + ctx.f14.f64));
	// fsubs f1,f14,f1
	ctx.f1.f64 = double(float(ctx.f14.f64 - ctx.f1.f64));
	// fmsubs f13,f13,f10,f23
	ctx.f13.f64 = double(float(std::fma(ctx.f13.f64, ctx.f10.f64, -ctx.f23.f64)));
	// fmadds f11,f9,f10,f11
	ctx.f11.f64 = double(float(std::fma(ctx.f9.f64, ctx.f10.f64, ctx.f11.f64)));
	// fadds f9,f30,f22
	ctx.f9.f64 = double(float(ctx.f30.f64 + ctx.f22.f64));
	// fadds f10,f13,f24
	ctx.f10.f64 = double(float(ctx.f13.f64 + ctx.f24.f64));
	// fsubs f13,f24,f13
	ctx.f13.f64 = double(float(ctx.f24.f64 - ctx.f13.f64));
	// fadds f24,f8,f31
	ctx.f24.f64 = double(float(ctx.f8.f64 + ctx.f31.f64));
	// fadds f23,f11,f12
	ctx.f23.f64 = double(float(ctx.f11.f64 + ctx.f12.f64));
	// fsubs f12,f12,f11
	ctx.f12.f64 = double(float(ctx.f12.f64 - ctx.f11.f64));
	// fadds f11,f26,f21
	ctx.f11.f64 = double(float(ctx.f26.f64 + ctx.f21.f64));
	// fsubs f8,f31,f8
	ctx.f8.f64 = double(float(ctx.f31.f64 - ctx.f8.f64));
	// fadds f15,f24,f9
	ctx.f15.f64 = double(float(ctx.f24.f64 + ctx.f9.f64));
	// stfs f15,0(r3)
	temp.f32 = float(ctx.f15.f64);
	REX_STORE_U32(ctx.r3.u32 + 0, temp.u32);
	// fadds f15,f5,f19
	ctx.f15.f64 = double(float(ctx.f5.f64 + ctx.f19.f64));
	// fsubs f9,f9,f24
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f24.f64));
	// stfs f9,8(r3)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r3.u32 + 8, temp.u32);
	// fadds f9,f18,f11
	ctx.f9.f64 = double(float(ctx.f18.f64 + ctx.f11.f64));
	// stfs f9,4(r3)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r3.u32 + 4, temp.u32);
	// fsubs f9,f21,f26
	ctx.f9.f64 = double(float(ctx.f21.f64 - ctx.f26.f64));
	// fsubs f11,f11,f18
	ctx.f11.f64 = double(float(ctx.f11.f64 - ctx.f18.f64));
	// stfs f11,12(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 12, temp.u32);
	// fsubs f11,f22,f30
	ctx.f11.f64 = double(float(ctx.f22.f64 - ctx.f30.f64));
	// fsubs f5,f19,f5
	ctx.f5.f64 = double(float(ctx.f19.f64 - ctx.f5.f64));
	// fadds f30,f15,f16
	ctx.f30.f64 = double(float(ctx.f15.f64 + ctx.f16.f64));
	// fsubs f31,f16,f15
	ctx.f31.f64 = double(float(ctx.f16.f64 - ctx.f15.f64));
	// fadds f26,f8,f9
	ctx.f26.f64 = double(float(ctx.f8.f64 + ctx.f9.f64));
	// stfs f26,20(r3)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r3.u32 + 20, temp.u32);
	// fsubs f9,f9,f8
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f8.f64));
	// stfs f9,28(r3)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r3.u32 + 28, temp.u32);
	// fsubs f9,f11,f1
	ctx.f9.f64 = double(float(ctx.f11.f64 - ctx.f1.f64));
	// stfs f9,16(r3)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r3.u32 + 16, temp.u32);
	// fadds f11,f1,f11
	ctx.f11.f64 = double(float(ctx.f1.f64 + ctx.f11.f64));
	// stfs f11,24(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 24, temp.u32);
	// fsubs f11,f4,f27
	ctx.f11.f64 = double(float(ctx.f4.f64 - ctx.f27.f64));
	// fadds f9,f2,f3
	ctx.f9.f64 = double(float(ctx.f2.f64 + ctx.f3.f64));
	// fadds f26,f5,f6
	ctx.f26.f64 = double(float(ctx.f5.f64 + ctx.f6.f64));
	// fmuls f1,f30,f0
	ctx.f1.f64 = double(float(ctx.f30.f64 * ctx.f0.f64));
	// fmuls f8,f31,f0
	ctx.f8.f64 = double(float(ctx.f31.f64 * ctx.f0.f64));
	// fsubs f30,f6,f5
	ctx.f30.f64 = double(float(ctx.f6.f64 - ctx.f5.f64));
	// fadds f25,f8,f11
	ctx.f25.f64 = double(float(ctx.f8.f64 + ctx.f11.f64));
	// lfs f31,-188(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -188);
	ctx.f31.f64 = double(temp.f32);
	// fsubs f11,f11,f8
	ctx.f11.f64 = double(float(ctx.f11.f64 - ctx.f8.f64));
	// stfs f11,40(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 40, temp.u32);
	// fadds f11,f1,f9
	ctx.f11.f64 = double(float(ctx.f1.f64 + ctx.f9.f64));
	// stfs f11,36(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 36, temp.u32);
	// fsubs f11,f9,f1
	ctx.f11.f64 = double(float(ctx.f9.f64 - ctx.f1.f64));
	// stfs f11,44(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 44, temp.u32);
	// fmuls f8,f30,f0
	ctx.f8.f64 = double(float(ctx.f30.f64 * ctx.f0.f64));
	// lfs f1,-180(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -180);
	ctx.f1.f64 = double(temp.f32);
	// fsubs f9,f3,f2
	ctx.f9.f64 = double(float(ctx.f3.f64 - ctx.f2.f64));
	// lfs f30,-176(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -176);
	ctx.f30.f64 = double(temp.f32);
	// fadds f11,f27,f4
	ctx.f11.f64 = double(float(ctx.f27.f64 + ctx.f4.f64));
	// lfs f2,-184(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -184);
	ctx.f2.f64 = double(temp.f32);
	// fmuls f4,f26,f0
	ctx.f4.f64 = double(float(ctx.f26.f64 * ctx.f0.f64));
	// stfs f25,32(r3)
	temp.f32 = float(ctx.f25.f64);
	REX_STORE_U32(ctx.r3.u32 + 32, temp.u32);
	// fsubs f5,f17,f13
	ctx.f5.f64 = double(float(ctx.f17.f64 - ctx.f13.f64));
	// fadds f6,f12,f31
	ctx.f6.f64 = double(float(ctx.f12.f64 + ctx.f31.f64));
	// fadds f13,f13,f17
	ctx.f13.f64 = double(float(ctx.f13.f64 + ctx.f17.f64));
	// fsubs f12,f31,f12
	ctx.f12.f64 = double(float(ctx.f31.f64 - ctx.f12.f64));
	// fadds f3,f8,f9
	ctx.f3.f64 = double(float(ctx.f8.f64 + ctx.f9.f64));
	// stfs f3,52(r3)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r3.u32 + 52, temp.u32);
	// fsubs f9,f9,f8
	ctx.f9.f64 = double(float(ctx.f9.f64 - ctx.f8.f64));
	// stfs f9,60(r3)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r3.u32 + 60, temp.u32);
	// fsubs f9,f11,f4
	ctx.f9.f64 = double(float(ctx.f11.f64 - ctx.f4.f64));
	// lfs f3,-200(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -200);
	ctx.f3.f64 = double(temp.f32);
	// fadds f11,f4,f11
	ctx.f11.f64 = double(float(ctx.f4.f64 + ctx.f11.f64));
	// stfs f11,56(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 56, temp.u32);
	// fsubs f8,f1,f10
	ctx.f8.f64 = double(float(ctx.f1.f64 - ctx.f10.f64));
	// stfs f9,48(r3)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r3.u32 + 48, temp.u32);
	// fadds f11,f3,f28
	ctx.f11.f64 = double(float(ctx.f3.f64 + ctx.f28.f64));
	// fadds f9,f2,f20
	ctx.f9.f64 = double(float(ctx.f2.f64 + ctx.f20.f64));
	// fsubs f4,f30,f23
	ctx.f4.f64 = double(float(ctx.f30.f64 - ctx.f23.f64));
	// fadds f10,f10,f1
	ctx.f10.f64 = double(float(ctx.f10.f64 + ctx.f1.f64));
	// fsubs f27,f6,f5
	ctx.f27.f64 = double(float(ctx.f6.f64 - ctx.f5.f64));
	// fadds f6,f5,f6
	ctx.f6.f64 = double(float(ctx.f5.f64 + ctx.f6.f64));
	// fadds f26,f8,f11
	ctx.f26.f64 = double(float(ctx.f8.f64 + ctx.f11.f64));
	// stfs f26,64(r3)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r3.u32 + 64, temp.u32);
	// fsubs f11,f11,f8
	ctx.f11.f64 = double(float(ctx.f11.f64 - ctx.f8.f64));
	// stfs f11,72(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 72, temp.u32);
	// fadds f11,f4,f9
	ctx.f11.f64 = double(float(ctx.f4.f64 + ctx.f9.f64));
	// stfs f11,68(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 68, temp.u32);
	// fsubs f11,f9,f4
	ctx.f11.f64 = double(float(ctx.f9.f64 - ctx.f4.f64));
	// stfs f11,76(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 76, temp.u32);
	// fsubs f9,f20,f2
	ctx.f9.f64 = double(float(ctx.f20.f64 - ctx.f2.f64));
	// fadds f8,f23,f30
	ctx.f8.f64 = double(float(ctx.f23.f64 + ctx.f30.f64));
	// fsubs f11,f28,f3
	ctx.f11.f64 = double(float(ctx.f28.f64 - ctx.f3.f64));
	// fadds f4,f10,f9
	ctx.f4.f64 = double(float(ctx.f10.f64 + ctx.f9.f64));
	// stfs f4,84(r3)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r3.u32 + 84, temp.u32);
	// fsubs f10,f9,f10
	ctx.f10.f64 = double(float(ctx.f9.f64 - ctx.f10.f64));
	// stfs f10,92(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 92, temp.u32);
	// fsubs f10,f11,f8
	ctx.f10.f64 = double(float(ctx.f11.f64 - ctx.f8.f64));
	// lfs f4,-172(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -172);
	ctx.f4.f64 = double(temp.f32);
	// fadds f11,f8,f11
	ctx.f11.f64 = double(float(ctx.f8.f64 + ctx.f11.f64));
	// stfs f11,88(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 88, temp.u32);
	// fmuls f9,f27,f0
	ctx.f9.f64 = double(float(ctx.f27.f64 * ctx.f0.f64));
	// lfs f8,-168(r1)
	temp.u32 = REX_LOAD_U32(ctx.r1.u32 + -168);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f11,f7,f4
	ctx.f11.f64 = double(float(ctx.f7.f64 - ctx.f4.f64));
	// stfs f10,80(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 80, temp.u32);
	// fadds f10,f8,f29
	ctx.f10.f64 = double(float(ctx.f8.f64 + ctx.f29.f64));
	// fadds f5,f9,f11
	ctx.f5.f64 = double(float(ctx.f9.f64 + ctx.f11.f64));
	// stfs f5,96(r3)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r3.u32 + 96, temp.u32);
	// fsubs f11,f11,f9
	ctx.f11.f64 = double(float(ctx.f11.f64 - ctx.f9.f64));
	// stfs f11,104(r3)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r3.u32 + 104, temp.u32);
	// fmuls f9,f6,f0
	ctx.f9.f64 = double(float(ctx.f6.f64 * ctx.f0.f64));
	// fadds f11,f4,f7
	ctx.f11.f64 = double(float(ctx.f4.f64 + ctx.f7.f64));
	// fsubs f7,f12,f13
	ctx.f7.f64 = double(float(ctx.f12.f64 - ctx.f13.f64));
	// fadds f6,f13,f12
	ctx.f6.f64 = double(float(ctx.f13.f64 + ctx.f12.f64));
	// fadds f13,f9,f10
	ctx.f13.f64 = double(float(ctx.f9.f64 + ctx.f10.f64));
	// stfs f13,100(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 100, temp.u32);
	// fsubs f13,f10,f9
	ctx.f13.f64 = double(float(ctx.f10.f64 - ctx.f9.f64));
	// stfs f13,108(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 108, temp.u32);
	// fmuls f12,f7,f0
	ctx.f12.f64 = double(float(ctx.f7.f64 * ctx.f0.f64));
	// fsubs f13,f29,f8
	ctx.f13.f64 = double(float(ctx.f29.f64 - ctx.f8.f64));
	// fmuls f0,f6,f0
	ctx.f0.f64 = double(float(ctx.f6.f64 * ctx.f0.f64));
	// fadds f10,f12,f13
	ctx.f10.f64 = double(float(ctx.f12.f64 + ctx.f13.f64));
	// stfs f10,116(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 116, temp.u32);
	// fsubs f10,f11,f0
	ctx.f10.f64 = double(float(ctx.f11.f64 - ctx.f0.f64));
	// stfs f10,112(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 112, temp.u32);
	// fadds f0,f0,f11
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f11.f64));
	// stfs f0,120(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 120, temp.u32);
	// fsubs f0,f13,f12
	ctx.f0.f64 = double(float(ctx.f13.f64 - ctx.f12.f64));
	// stfs f0,124(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 124, temp.u32);
	// addi r12,r1,-8
	ctx.r12.s64 = ctx.r1.s64 + -8;
	// bl 0x824144dc
	ctx.lr = 0x8295CCC8;
	__restfpr_14(ctx, base);
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_8295CCD8) {
	REX_FUNC_PROLOGUE(sub_8295CCD8);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// addi r12,r1,-8
	ctx.r12.s64 = ctx.r1.s64 + -8;
	// bl 0x824144b8
	ctx.lr = 0x8295CCE8;
	__savefpr_24(ctx, base);
	// lfs f12,32(r3)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 32);
	ctx.f12.f64 = double(temp.f32);
	// lfs f13,0(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// fadds f28,f13,f12
	ctx.f28.f64 = double(float(ctx.f13.f64 + ctx.f12.f64));
	// lfs f10,4(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 4);
	ctx.f10.f64 = double(temp.f32);
	// lfs f11,36(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 36);
	ctx.f11.f64 = double(temp.f32);
	// fsubs f13,f13,f12
	ctx.f13.f64 = double(float(ctx.f13.f64 - ctx.f12.f64));
	// lfs f9,48(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 48);
	ctx.f9.f64 = double(temp.f32);
	// fadds f12,f10,f11
	ctx.f12.f64 = double(float(ctx.f10.f64 + ctx.f11.f64));
	// lfs f8,16(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 16);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f11,f10,f11
	ctx.f11.f64 = double(float(ctx.f10.f64 - ctx.f11.f64));
	// fadds f10,f8,f9
	ctx.f10.f64 = double(float(ctx.f8.f64 + ctx.f9.f64));
	// lfs f7,52(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 52);
	ctx.f7.f64 = double(temp.f32);
	// lfs f6,20(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 20);
	ctx.f6.f64 = double(temp.f32);
	// fsubs f9,f8,f9
	ctx.f9.f64 = double(float(ctx.f8.f64 - ctx.f9.f64));
	// fadds f8,f6,f7
	ctx.f8.f64 = double(float(ctx.f6.f64 + ctx.f7.f64));
	// lfs f5,40(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 40);
	ctx.f5.f64 = double(temp.f32);
	// fsubs f7,f6,f7
	ctx.f7.f64 = double(float(ctx.f6.f64 - ctx.f7.f64));
	// lfs f3,8(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 8);
	ctx.f3.f64 = double(temp.f32);
	// lfs f4,44(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 44);
	ctx.f4.f64 = double(temp.f32);
	// lfs f2,12(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 12);
	ctx.f2.f64 = double(temp.f32);
	// lfs f1,56(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 56);
	ctx.f1.f64 = double(temp.f32);
	// fadds f26,f2,f4
	ctx.f26.f64 = double(float(ctx.f2.f64 + ctx.f4.f64));
	// lfs f30,60(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 60);
	ctx.f30.f64 = double(temp.f32);
	// lfs f31,24(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 24);
	ctx.f31.f64 = double(temp.f32);
	// lfs f29,28(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 28);
	ctx.f29.f64 = double(temp.f32);
	// fadds f25,f31,f1
	ctx.f25.f64 = double(float(ctx.f31.f64 + ctx.f1.f64));
	// fadds f6,f10,f28
	ctx.f6.f64 = double(float(ctx.f10.f64 + ctx.f28.f64));
	// lfs f0,4(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 4);
	ctx.f0.f64 = double(temp.f32);
	// fsubs f10,f28,f10
	ctx.f10.f64 = double(float(ctx.f28.f64 - ctx.f10.f64));
	// fadds f28,f8,f12
	ctx.f28.f64 = double(float(ctx.f8.f64 + ctx.f12.f64));
	// fsubs f12,f12,f8
	ctx.f12.f64 = double(float(ctx.f12.f64 - ctx.f8.f64));
	// fsubs f8,f13,f7
	ctx.f8.f64 = double(float(ctx.f13.f64 - ctx.f7.f64));
	// fadds f27,f9,f11
	ctx.f27.f64 = double(float(ctx.f9.f64 + ctx.f11.f64));
	// fsubs f11,f11,f9
	ctx.f11.f64 = double(float(ctx.f11.f64 - ctx.f9.f64));
	// fadds f13,f7,f13
	ctx.f13.f64 = double(float(ctx.f7.f64 + ctx.f13.f64));
	// fadds f7,f3,f5
	ctx.f7.f64 = double(float(ctx.f3.f64 + ctx.f5.f64));
	// fsubs f9,f3,f5
	ctx.f9.f64 = double(float(ctx.f3.f64 - ctx.f5.f64));
	// fsubs f5,f2,f4
	ctx.f5.f64 = double(float(ctx.f2.f64 - ctx.f4.f64));
	// fsubs f3,f29,f30
	ctx.f3.f64 = double(float(ctx.f29.f64 - ctx.f30.f64));
	// fsubs f4,f31,f1
	ctx.f4.f64 = double(float(ctx.f31.f64 - ctx.f1.f64));
	// fadds f24,f29,f30
	ctx.f24.f64 = double(float(ctx.f29.f64 + ctx.f30.f64));
	// fadds f2,f25,f7
	ctx.f2.f64 = double(float(ctx.f25.f64 + ctx.f7.f64));
	// fsubs f7,f7,f25
	ctx.f7.f64 = double(float(ctx.f7.f64 - ctx.f25.f64));
	// fsubs f29,f9,f3
	ctx.f29.f64 = double(float(ctx.f9.f64 - ctx.f3.f64));
	// fadds f30,f4,f5
	ctx.f30.f64 = double(float(ctx.f4.f64 + ctx.f5.f64));
	// fadds f9,f3,f9
	ctx.f9.f64 = double(float(ctx.f3.f64 + ctx.f9.f64));
	// fsubs f5,f5,f4
	ctx.f5.f64 = double(float(ctx.f5.f64 - ctx.f4.f64));
	// fadds f1,f24,f26
	ctx.f1.f64 = double(float(ctx.f24.f64 + ctx.f26.f64));
	// fsubs f31,f26,f24
	ctx.f31.f64 = double(float(ctx.f26.f64 - ctx.f24.f64));
	// fsubs f4,f29,f30
	ctx.f4.f64 = double(float(ctx.f29.f64 - ctx.f30.f64));
	// fadds f3,f30,f29
	ctx.f3.f64 = double(float(ctx.f30.f64 + ctx.f29.f64));
	// fadds f30,f5,f9
	ctx.f30.f64 = double(float(ctx.f5.f64 + ctx.f9.f64));
	// fsubs f29,f9,f5
	ctx.f29.f64 = double(float(ctx.f9.f64 - ctx.f5.f64));
	// fmuls f9,f4,f0
	ctx.f9.f64 = double(float(ctx.f4.f64 * ctx.f0.f64));
	// fmuls f5,f3,f0
	ctx.f5.f64 = double(float(ctx.f3.f64 * ctx.f0.f64));
	// fmuls f4,f30,f0
	ctx.f4.f64 = double(float(ctx.f30.f64 * ctx.f0.f64));
	// fmuls f0,f29,f0
	ctx.f0.f64 = double(float(ctx.f29.f64 * ctx.f0.f64));
	// fadds f3,f9,f8
	ctx.f3.f64 = double(float(ctx.f9.f64 + ctx.f8.f64));
	// stfs f3,32(r3)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r3.u32 + 32, temp.u32);
	// fsubs f9,f8,f9
	ctx.f9.f64 = double(float(ctx.f8.f64 - ctx.f9.f64));
	// stfs f9,40(r3)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r3.u32 + 40, temp.u32);
	// fsubs f9,f27,f5
	ctx.f9.f64 = double(float(ctx.f27.f64 - ctx.f5.f64));
	// stfs f9,44(r3)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r3.u32 + 44, temp.u32);
	// fsubs f9,f13,f4
	ctx.f9.f64 = double(float(ctx.f13.f64 - ctx.f4.f64));
	// stfs f9,48(r3)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r3.u32 + 48, temp.u32);
	// fadds f13,f4,f13
	ctx.f13.f64 = double(float(ctx.f4.f64 + ctx.f13.f64));
	// stfs f13,56(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 56, temp.u32);
	// fadds f8,f0,f11
	ctx.f8.f64 = double(float(ctx.f0.f64 + ctx.f11.f64));
	// stfs f8,52(r3)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r3.u32 + 52, temp.u32);
	// fsubs f0,f11,f0
	ctx.f0.f64 = double(float(ctx.f11.f64 - ctx.f0.f64));
	// stfs f0,60(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 60, temp.u32);
	// fadds f13,f2,f6
	ctx.f13.f64 = double(float(ctx.f2.f64 + ctx.f6.f64));
	// stfs f13,0(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 0, temp.u32);
	// fadds f0,f1,f28
	ctx.f0.f64 = double(float(ctx.f1.f64 + ctx.f28.f64));
	// stfs f0,4(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 4, temp.u32);
	// fsubs f13,f6,f2
	ctx.f13.f64 = double(float(ctx.f6.f64 - ctx.f2.f64));
	// stfs f13,8(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 8, temp.u32);
	// fadds f3,f5,f27
	ctx.f3.f64 = double(float(ctx.f5.f64 + ctx.f27.f64));
	// stfs f3,36(r3)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r3.u32 + 36, temp.u32);
	// fsubs f0,f28,f1
	ctx.f0.f64 = double(float(ctx.f28.f64 - ctx.f1.f64));
	// fsubs f13,f10,f31
	ctx.f13.f64 = double(float(ctx.f10.f64 - ctx.f31.f64));
	// stfs f0,12(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 12, temp.u32);
	// fadds f0,f7,f12
	ctx.f0.f64 = double(float(ctx.f7.f64 + ctx.f12.f64));
	// stfs f13,16(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 16, temp.u32);
	// fadds f13,f31,f10
	ctx.f13.f64 = double(float(ctx.f31.f64 + ctx.f10.f64));
	// stfs f0,20(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 20, temp.u32);
	// fsubs f0,f12,f7
	ctx.f0.f64 = double(float(ctx.f12.f64 - ctx.f7.f64));
	// stfs f13,24(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 24, temp.u32);
	// stfs f0,28(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 28, temp.u32);
	// addi r12,r1,-8
	ctx.r12.s64 = ctx.r1.s64 + -8;
	// bl 0x82414504
	ctx.lr = 0x8295CE54;
	__restfpr_24(ctx, base);
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_8295CE60) {
	REX_FUNC_PROLOGUE(sub_8295CE60);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// addi r12,r1,-8
	ctx.r12.s64 = ctx.r1.s64 + -8;
	// bl 0x824144ac
	ctx.lr = 0x8295CE70;
	__savefpr_21(ctx, base);
	// lfs f9,52(r3)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 52);
	ctx.f9.f64 = double(temp.f32);
	// lfs f8,48(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 48);
	ctx.f8.f64 = double(temp.f32);
	// lfs f7,20(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 20);
	ctx.f7.f64 = double(temp.f32);
	// lfs f10,16(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 16);
	ctx.f10.f64 = double(temp.f32);
	// fadds f25,f7,f8
	ctx.f25.f64 = double(float(ctx.f7.f64 + ctx.f8.f64));
	// fsubs f26,f10,f9
	ctx.f26.f64 = double(float(ctx.f10.f64 - ctx.f9.f64));
	// lfs f11,0(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f29,36(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 36);
	ctx.f29.f64 = double(temp.f32);
	// fadds f10,f9,f10
	ctx.f10.f64 = double(float(ctx.f9.f64 + ctx.f10.f64));
	// lfs f27,4(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 4);
	ctx.f27.f64 = double(temp.f32);
	// fsubs f24,f11,f29
	ctx.f24.f64 = double(float(ctx.f11.f64 - ctx.f29.f64));
	// lfs f28,32(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 32);
	ctx.f28.f64 = double(temp.f32);
	// fadds f11,f29,f11
	ctx.f11.f64 = double(float(ctx.f29.f64 + ctx.f11.f64));
	// fadds f29,f27,f28
	ctx.f29.f64 = double(float(ctx.f27.f64 + ctx.f28.f64));
	// lfs f12,4(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// fsubs f28,f27,f28
	ctx.f28.f64 = double(float(ctx.f27.f64 - ctx.f28.f64));
	// lfs f6,8(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 8);
	ctx.f6.f64 = double(temp.f32);
	// fsubs f9,f7,f8
	ctx.f9.f64 = double(float(ctx.f7.f64 - ctx.f8.f64));
	// lfs f3,12(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 12);
	ctx.f3.f64 = double(temp.f32);
	// lfs f5,44(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 44);
	ctx.f5.f64 = double(temp.f32);
	// lfs f4,40(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 40);
	ctx.f4.f64 = double(temp.f32);
	// lfs f0,16(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 16);
	ctx.f0.f64 = double(temp.f32);
	// fsubs f27,f26,f25
	ctx.f27.f64 = double(float(ctx.f26.f64 - ctx.f25.f64));
	// lfs f13,20(r4)
	temp.u32 = REX_LOAD_U32(ctx.r4.u32 + 20);
	ctx.f13.f64 = double(temp.f32);
	// fadds f26,f25,f26
	ctx.f26.f64 = double(float(ctx.f25.f64 + ctx.f26.f64));
	// lfs f2,24(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 24);
	ctx.f2.f64 = double(temp.f32);
	// lfs f1,60(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 60);
	ctx.f1.f64 = double(temp.f32);
	// lfs f31,56(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 56);
	ctx.f31.f64 = double(temp.f32);
	// lfs f30,28(r3)
	temp.u32 = REX_LOAD_U32(ctx.r3.u32 + 28);
	ctx.f30.f64 = double(temp.f32);
	// fmuls f8,f27,f12
	ctx.f8.f64 = double(float(ctx.f27.f64 * ctx.f12.f64));
	// fsubs f27,f10,f9
	ctx.f27.f64 = double(float(ctx.f10.f64 - ctx.f9.f64));
	// fmuls f7,f26,f12
	ctx.f7.f64 = double(float(ctx.f26.f64 * ctx.f12.f64));
	// fadds f26,f9,f10
	ctx.f26.f64 = double(float(ctx.f9.f64 + ctx.f10.f64));
	// fsubs f10,f6,f5
	ctx.f10.f64 = double(float(ctx.f6.f64 - ctx.f5.f64));
	// fadds f9,f3,f4
	ctx.f9.f64 = double(float(ctx.f3.f64 + ctx.f4.f64));
	// fmuls f27,f27,f12
	ctx.f27.f64 = double(float(ctx.f27.f64 * ctx.f12.f64));
	// fmuls f12,f26,f12
	ctx.f12.f64 = double(float(ctx.f26.f64 * ctx.f12.f64));
	// fmr f26,f10
	ctx.f26.f64 = ctx.f10.f64;
	// fmr f25,f10
	ctx.f25.f64 = ctx.f10.f64;
	// fmuls f23,f9,f13
	ctx.f23.f64 = double(float(ctx.f9.f64 * ctx.f13.f64));
	// fmuls f22,f9,f0
	ctx.f22.f64 = double(float(ctx.f9.f64 * ctx.f0.f64));
	// fadds f10,f5,f6
	ctx.f10.f64 = double(float(ctx.f5.f64 + ctx.f6.f64));
	// fsubs f9,f3,f4
	ctx.f9.f64 = double(float(ctx.f3.f64 - ctx.f4.f64));
	// fmsubs f6,f26,f0,f23
	ctx.f6.f64 = double(float(std::fma(ctx.f26.f64, ctx.f0.f64, -ctx.f23.f64)));
	// fmadds f5,f25,f13,f22
	ctx.f5.f64 = double(float(std::fma(ctx.f25.f64, ctx.f13.f64, ctx.f22.f64)));
	// fmuls f3,f10,f0
	ctx.f3.f64 = double(float(ctx.f10.f64 * ctx.f0.f64));
	// fmr f4,f10
	ctx.f4.f64 = ctx.f10.f64;
	// fmuls f26,f9,f0
	ctx.f26.f64 = double(float(ctx.f9.f64 * ctx.f0.f64));
	// fmr f25,f9
	ctx.f25.f64 = ctx.f9.f64;
	// fsubs f10,f2,f1
	ctx.f10.f64 = double(float(ctx.f2.f64 - ctx.f1.f64));
	// fadds f9,f30,f31
	ctx.f9.f64 = double(float(ctx.f30.f64 + ctx.f31.f64));
	// fmsubs f4,f4,f13,f26
	ctx.f4.f64 = double(float(std::fma(ctx.f4.f64, ctx.f13.f64, -ctx.f26.f64)));
	// fmadds f3,f25,f13,f3
	ctx.f3.f64 = double(float(std::fma(ctx.f25.f64, ctx.f13.f64, ctx.f3.f64)));
	// fmr f26,f10
	ctx.f26.f64 = ctx.f10.f64;
	// fmuls f23,f9,f0
	ctx.f23.f64 = double(float(ctx.f9.f64 * ctx.f0.f64));
	// fmuls f25,f10,f0
	ctx.f25.f64 = double(float(ctx.f10.f64 * ctx.f0.f64));
	// fadds f10,f1,f2
	ctx.f10.f64 = double(float(ctx.f1.f64 + ctx.f2.f64));
	// fsubs f1,f30,f31
	ctx.f1.f64 = double(float(ctx.f30.f64 - ctx.f31.f64));
	// fmsubs f2,f26,f13,f23
	ctx.f2.f64 = double(float(std::fma(ctx.f26.f64, ctx.f13.f64, -ctx.f23.f64)));
	// fmadds f9,f9,f13,f25
	ctx.f9.f64 = double(float(std::fma(ctx.f9.f64, ctx.f13.f64, ctx.f25.f64)));
	// fmr f26,f10
	ctx.f26.f64 = ctx.f10.f64;
	// fmr f25,f10
	ctx.f25.f64 = ctx.f10.f64;
	// fadds f10,f8,f24
	ctx.f10.f64 = double(float(ctx.f8.f64 + ctx.f24.f64));
	// fmuls f23,f1,f13
	ctx.f23.f64 = double(float(ctx.f1.f64 * ctx.f13.f64));
	// fmuls f22,f1,f0
	ctx.f22.f64 = double(float(ctx.f1.f64 * ctx.f0.f64));
	// fadds f1,f7,f29
	ctx.f1.f64 = double(float(ctx.f7.f64 + ctx.f29.f64));
	// fsubs f7,f29,f7
	ctx.f7.f64 = double(float(ctx.f29.f64 - ctx.f7.f64));
	// fadds f31,f2,f6
	ctx.f31.f64 = double(float(ctx.f2.f64 + ctx.f6.f64));
	// fadds f30,f9,f5
	ctx.f30.f64 = double(float(ctx.f9.f64 + ctx.f5.f64));
	// fmsubs f0,f26,f0,f23
	ctx.f0.f64 = double(float(std::fma(ctx.f26.f64, ctx.f0.f64, -ctx.f23.f64)));
	// fmadds f13,f25,f13,f22
	ctx.f13.f64 = double(float(std::fma(ctx.f25.f64, ctx.f13.f64, ctx.f22.f64)));
	// fadds f21,f31,f10
	ctx.f21.f64 = double(float(ctx.f31.f64 + ctx.f10.f64));
	// stfs f21,0(r3)
	temp.f32 = float(ctx.f21.f64);
	REX_STORE_U32(ctx.r3.u32 + 0, temp.u32);
	// fsubs f10,f10,f31
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f31.f64));
	// stfs f10,8(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 8, temp.u32);
	// fsubs f10,f24,f8
	ctx.f10.f64 = double(float(ctx.f24.f64 - ctx.f8.f64));
	// fsubs f8,f6,f2
	ctx.f8.f64 = double(float(ctx.f6.f64 - ctx.f2.f64));
	// fadds f6,f30,f1
	ctx.f6.f64 = double(float(ctx.f30.f64 + ctx.f1.f64));
	// stfs f6,4(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 4, temp.u32);
	// fsubs f6,f1,f30
	ctx.f6.f64 = double(float(ctx.f1.f64 - ctx.f30.f64));
	// stfs f6,12(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 12, temp.u32);
	// fsubs f9,f5,f9
	ctx.f9.f64 = double(float(ctx.f5.f64 - ctx.f9.f64));
	// fadds f6,f8,f7
	ctx.f6.f64 = double(float(ctx.f8.f64 + ctx.f7.f64));
	// stfs f6,20(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 20, temp.u32);
	// fsubs f6,f10,f9
	ctx.f6.f64 = double(float(ctx.f10.f64 - ctx.f9.f64));
	// stfs f6,16(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 16, temp.u32);
	// fadds f10,f9,f10
	ctx.f10.f64 = double(float(ctx.f9.f64 + ctx.f10.f64));
	// stfs f10,24(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 24, temp.u32);
	// fsubs f10,f7,f8
	ctx.f10.f64 = double(float(ctx.f7.f64 - ctx.f8.f64));
	// stfs f10,28(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 28, temp.u32);
	// fsubs f10,f11,f12
	ctx.f10.f64 = double(float(ctx.f11.f64 - ctx.f12.f64));
	// fsubs f8,f4,f0
	ctx.f8.f64 = double(float(ctx.f4.f64 - ctx.f0.f64));
	// fsubs f7,f3,f13
	ctx.f7.f64 = double(float(ctx.f3.f64 - ctx.f13.f64));
	// fadds f9,f27,f28
	ctx.f9.f64 = double(float(ctx.f27.f64 + ctx.f28.f64));
	// fadds f12,f12,f11
	ctx.f12.f64 = double(float(ctx.f12.f64 + ctx.f11.f64));
	// fsubs f11,f28,f27
	ctx.f11.f64 = double(float(ctx.f28.f64 - ctx.f27.f64));
	// fadds f0,f0,f4
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f4.f64));
	// fadds f13,f13,f3
	ctx.f13.f64 = double(float(ctx.f13.f64 + ctx.f3.f64));
	// fadds f6,f8,f10
	ctx.f6.f64 = double(float(ctx.f8.f64 + ctx.f10.f64));
	// stfs f6,32(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 32, temp.u32);
	// fsubs f10,f10,f8
	ctx.f10.f64 = double(float(ctx.f10.f64 - ctx.f8.f64));
	// stfs f10,40(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 40, temp.u32);
	// fsubs f10,f9,f7
	ctx.f10.f64 = double(float(ctx.f9.f64 - ctx.f7.f64));
	// stfs f10,44(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 44, temp.u32);
	// fadds f6,f7,f9
	ctx.f6.f64 = double(float(ctx.f7.f64 + ctx.f9.f64));
	// stfs f6,36(r3)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r3.u32 + 36, temp.u32);
	// fadds f10,f0,f11
	ctx.f10.f64 = double(float(ctx.f0.f64 + ctx.f11.f64));
	// stfs f10,52(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 52, temp.u32);
	// fsubs f10,f12,f13
	ctx.f10.f64 = double(float(ctx.f12.f64 - ctx.f13.f64));
	// stfs f10,48(r3)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r3.u32 + 48, temp.u32);
	// fadds f13,f13,f12
	ctx.f13.f64 = double(float(ctx.f13.f64 + ctx.f12.f64));
	// stfs f13,56(r3)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r3.u32 + 56, temp.u32);
	// fsubs f0,f11,f0
	ctx.f0.f64 = double(float(ctx.f11.f64 - ctx.f0.f64));
	// stfs f0,60(r3)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r3.u32 + 60, temp.u32);
	// addi r12,r1,-8
	ctx.r12.s64 = ctx.r1.s64 + -8;
	// bl 0x824144f8
	ctx.lr = 0x8295D040;
	__restfpr_21(ctx, base);
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_8295D050) {
	REX_FUNC_PROLOGUE(sub_8295D050);
	PPCRegister temp{};
	// srawi r10,r3,1
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0x1) != 0);
	ctx.r10.s64 = ctx.r3.s32 >> 1;
	// rlwinm r11,r5,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 1) & 0xFFFFFFFE;
	// mr r8,r6
	ctx.r8.u64 = ctx.r6.u64;
	// divw r11,r11,r10
	ctx.r11.u64 = uint32_t((ctx.r10.s32 && !(ctx.r11.s32 == INT32_MIN && ctx.r10.s32 == -1)) ? ctx.r11.s32 / ctx.r10.s32 : 0);
	// cmpwi cr6,r10,2
	ctx.cr6.compare<int32_t>(ctx.r10.s32, 2, ctx.xer);
	// blelr cr6
	if (!ctx.cr6.gt) return;
	// addi r9,r3,-2
	ctx.r9.s64 = ctx.r3.s64 + -2;
	// neg r3,r11
	ctx.r3.s64 = static_cast<int64_t>(-ctx.r11.u64);
	// rlwinm r6,r9,2,0,29
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r5,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r5,r11,2,0,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r11,r6,r4
	ctx.r11.u64 = ctx.r6.u64 + ctx.r4.u64;
	// addi r10,r10,-3
	ctx.r10.s64 = ctx.r10.s64 + -3;
	// lis r6,-32256
	ctx.r6.s64 = -2113929216;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r10,r4,8
	ctx.r10.s64 = ctx.r4.s64 + 8;
	// rlwinm r3,r3,2,0,29
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r9,r8
	ctx.r9.u64 = ctx.r9.u64 + ctx.r8.u64;
	// lfs f8,3636(r6)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 3636);
	ctx.f8.f64 = double(temp.f32);
	// addi r7,r7,1
	ctx.r7.s64 = ctx.r7.s64 + 1;
loc_8295D0A0:
	// add r9,r3,r9
	ctx.r9.u64 = ctx.r3.u64 + ctx.r9.u64;
	// lfs f13,4(r10)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f9,4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4);
	ctx.f9.f64 = double(temp.f32);
	// add r8,r5,r8
	ctx.r8.u64 = ctx.r5.u64 + ctx.r8.u64;
	// fadds f9,f9,f13
	ctx.f9.f64 = double(float(ctx.f9.f64 + ctx.f13.f64));
	// lfs f0,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f12,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// addi r7,r7,-1
	ctx.r7.s64 = ctx.r7.s64 + -1;
	// fsubs f12,f0,f12
	ctx.f12.f64 = double(float(ctx.f0.f64 - ctx.f12.f64));
	// lfs f10,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f10.f64 = double(temp.f32);
	// cmplwi cr6,r7,0
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 0, ctx.xer);
	// fsubs f10,f8,f10
	ctx.f10.f64 = double(float(ctx.f8.f64 - ctx.f10.f64));
	// lfs f11,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fmuls f7,f9,f11
	ctx.f7.f64 = double(float(ctx.f9.f64 * ctx.f11.f64));
	// fmuls f9,f9,f10
	ctx.f9.f64 = double(float(ctx.f9.f64 * ctx.f10.f64));
	// fmsubs f10,f12,f10,f7
	ctx.f10.f64 = double(float(std::fma(ctx.f12.f64, ctx.f10.f64, -ctx.f7.f64)));
	// fmadds f12,f12,f11,f9
	ctx.f12.f64 = double(float(std::fma(ctx.f12.f64, ctx.f11.f64, ctx.f9.f64)));
	// fsubs f0,f0,f10
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f10.f64));
	// stfs f0,0(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// fsubs f0,f13,f12
	ctx.f0.f64 = double(float(ctx.f13.f64 - ctx.f12.f64));
	// stfs f0,4(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// lfs f0,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// addi r10,r10,8
	ctx.r10.s64 = ctx.r10.s64 + 8;
	// lfs f13,4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// fadds f0,f0,f10
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f10.f64));
	// stfs f0,0(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// fsubs f0,f13,f12
	ctx.f0.f64 = double(float(ctx.f13.f64 - ctx.f12.f64));
	// stfs f0,4(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 4, temp.u32);
	// addi r11,r11,-8
	ctx.r11.s64 = ctx.r11.s64 + -8;
	// bne cr6,0x8295d0a0
	if (!ctx.cr6.eq) goto loc_8295D0A0;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_8295D120) {
	REX_FUNC_PROLOGUE(sub_8295D120);
	PPCRegister temp{};
	// srawi r10,r3,1
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0x1) != 0);
	ctx.r10.s64 = ctx.r3.s32 >> 1;
	// rlwinm r11,r5,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 1) & 0xFFFFFFFE;
	// mr r8,r6
	ctx.r8.u64 = ctx.r6.u64;
	// divw r11,r11,r10
	ctx.r11.u64 = uint32_t((ctx.r10.s32 && !(ctx.r11.s32 == INT32_MIN && ctx.r10.s32 == -1)) ? ctx.r11.s32 / ctx.r10.s32 : 0);
	// cmpwi cr6,r10,2
	ctx.cr6.compare<int32_t>(ctx.r10.s32, 2, ctx.xer);
	// blelr cr6
	if (!ctx.cr6.gt) return;
	// addi r9,r3,-2
	ctx.r9.s64 = ctx.r3.s64 + -2;
	// neg r3,r11
	ctx.r3.s64 = static_cast<int64_t>(-ctx.r11.u64);
	// rlwinm r6,r9,2,0,29
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r5,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r5,r11,2,0,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r11,r6,r4
	ctx.r11.u64 = ctx.r6.u64 + ctx.r4.u64;
	// addi r10,r10,-3
	ctx.r10.s64 = ctx.r10.s64 + -3;
	// lis r6,-32256
	ctx.r6.s64 = -2113929216;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r10,r4,8
	ctx.r10.s64 = ctx.r4.s64 + 8;
	// rlwinm r3,r3,2,0,29
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r9,r8
	ctx.r9.u64 = ctx.r9.u64 + ctx.r8.u64;
	// lfs f8,3636(r6)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 3636);
	ctx.f8.f64 = double(temp.f32);
	// addi r7,r7,1
	ctx.r7.s64 = ctx.r7.s64 + 1;
loc_8295D170:
	// add r9,r3,r9
	ctx.r9.u64 = ctx.r3.u64 + ctx.r9.u64;
	// lfs f0,0(r10)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f12,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// add r8,r5,r8
	ctx.r8.u64 = ctx.r5.u64 + ctx.r8.u64;
	// fsubs f12,f0,f12
	ctx.f12.f64 = double(float(ctx.f0.f64 - ctx.f12.f64));
	// lfs f13,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// lfs f9,4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4);
	ctx.f9.f64 = double(temp.f32);
	// addi r7,r7,-1
	ctx.r7.s64 = ctx.r7.s64 + -1;
	// fadds f9,f9,f13
	ctx.f9.f64 = double(float(ctx.f9.f64 + ctx.f13.f64));
	// lfs f10,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f10.f64 = double(temp.f32);
	// cmplwi cr6,r7,0
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 0, ctx.xer);
	// fsubs f10,f8,f10
	ctx.f10.f64 = double(float(ctx.f8.f64 - ctx.f10.f64));
	// lfs f11,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fmuls f6,f12,f11
	ctx.f6.f64 = double(float(ctx.f12.f64 * ctx.f11.f64));
	// fmuls f7,f12,f10
	ctx.f7.f64 = double(float(ctx.f12.f64 * ctx.f10.f64));
	// fmadds f12,f9,f11,f7
	ctx.f12.f64 = double(float(std::fma(ctx.f9.f64, ctx.f11.f64, ctx.f7.f64)));
	// fmsubs f11,f9,f10,f6
	ctx.f11.f64 = double(float(std::fma(ctx.f9.f64, ctx.f10.f64, -ctx.f6.f64)));
	// fsubs f0,f0,f12
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f12.f64));
	// stfs f0,0(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// fsubs f0,f13,f11
	ctx.f0.f64 = double(float(ctx.f13.f64 - ctx.f11.f64));
	// stfs f0,4(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// lfs f0,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// addi r10,r10,8
	ctx.r10.s64 = ctx.r10.s64 + 8;
	// lfs f13,4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// fadds f0,f0,f12
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f12.f64));
	// stfs f0,0(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// fsubs f0,f13,f11
	ctx.f0.f64 = double(float(ctx.f13.f64 - ctx.f11.f64));
	// stfs f0,4(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 4, temp.u32);
	// addi r11,r11,-8
	ctx.r11.s64 = ctx.r11.s64 + -8;
	// bne cr6,0x8295d170
	if (!ctx.cr6.eq) goto loc_8295D170;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_8295D1F0) {
	REX_FUNC_PROLOGUE(sub_8295D1F0);
	PPCRegister temp{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413198
	ctx.lr = 0x8295D1F8;
	__savegprlr_24(ctx, base);
	// srawi r24,r3,1
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0x1) != 0);
	ctx.r24.s64 = ctx.r3.s32 >> 1;
	// divw r9,r5,r3
	ctx.r9.u64 = uint32_t((ctx.r3.s32 && !(ctx.r5.s32 == INT32_MIN && ctx.r3.s32 == -1)) ? ctx.r5.s32 / ctx.r3.s32 : 0);
	// addi r11,r24,-1
	ctx.r11.s64 = ctx.r24.s64 + -1;
	// li r26,0
	ctx.r26.s64 = 0;
	// li r25,1
	ctx.r25.s64 = 1;
	// cmpwi cr6,r11,4
	ctx.cr6.compare<int32_t>(ctx.r11.s32, 4, ctx.xer);
	// blt cr6,0x8295d35c
	if (ctx.cr6.lt) goto loc_8295D35C;
	// addi r11,r24,-5
	ctx.r11.s64 = ctx.r24.s64 + -5;
	// addi r10,r3,-3
	ctx.r10.s64 = ctx.r3.s64 + -3;
	// rlwinm r11,r11,30,2,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 30) & 0x3FFFFFFF;
	// rlwinm r29,r5,2,0,29
	ctx.r29.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r31,r11,1
	ctx.r31.s64 = ctx.r11.s64 + 1;
	// neg r11,r9
	ctx.r11.s64 = static_cast<int64_t>(-ctx.r9.u64);
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r28,r31,2,0,29
	ctx.r28.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r7,r11,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r9,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r30,r6
	ctx.r30.u64 = ctx.r6.u64;
	// addi r11,r4,12
	ctx.r11.s64 = ctx.r4.s64 + 12;
	// add r29,r29,r6
	ctx.r29.u64 = ctx.r29.u64 + ctx.r6.u64;
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// addi r25,r28,1
	ctx.r25.s64 = ctx.r28.s64 + 1;
loc_8295D250:
	// add r29,r7,r29
	ctx.r29.u64 = ctx.r7.u64 + ctx.r29.u64;
	// lfs f11,8(r10)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 8);
	ctx.f11.f64 = double(temp.f32);
	// add r30,r30,r8
	ctx.r30.u64 = ctx.r30.u64 + ctx.r8.u64;
	// lfs f12,-8(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -8);
	ctx.f12.f64 = double(temp.f32);
	// addi r27,r10,-4
	ctx.r27.s64 = ctx.r10.s64 + -4;
	// addi r28,r11,4
	ctx.r28.s64 = ctx.r11.s64 + 4;
	// add r26,r26,r9
	ctx.r26.u64 = ctx.r26.u64 + ctx.r9.u64;
	// lfs f0,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// add r29,r7,r29
	ctx.r29.u64 = ctx.r7.u64 + ctx.r29.u64;
	// lfs f13,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// add r30,r30,r8
	ctx.r30.u64 = ctx.r30.u64 + ctx.r8.u64;
	// fsubs f10,f13,f0
	ctx.f10.f64 = double(float(ctx.f13.f64 - ctx.f0.f64));
	// add r26,r26,r9
	ctx.r26.u64 = ctx.r26.u64 + ctx.r9.u64;
	// fadds f0,f13,f0
	ctx.f0.f64 = double(float(ctx.f13.f64 + ctx.f0.f64));
	// addi r31,r31,-1
	ctx.r31.s64 = ctx.r31.s64 + -1;
	// add r26,r26,r9
	ctx.r26.u64 = ctx.r26.u64 + ctx.r9.u64;
	// cmplwi cr6,r31,0
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, 0, ctx.xer);
	// add r26,r26,r9
	ctx.r26.u64 = ctx.r26.u64 + ctx.r9.u64;
	// fmuls f13,f11,f10
	ctx.f13.f64 = double(float(ctx.f11.f64 * ctx.f10.f64));
	// fmuls f11,f11,f0
	ctx.f11.f64 = double(float(ctx.f11.f64 * ctx.f0.f64));
	// fmsubs f0,f12,f0,f13
	ctx.f0.f64 = double(float(std::fma(ctx.f12.f64, ctx.f0.f64, -ctx.f13.f64)));
	// fmadds f13,f12,f10,f11
	ctx.f13.f64 = double(float(std::fma(ctx.f12.f64, ctx.f10.f64, ctx.f11.f64)));
	// stfs f13,-8(r11)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r11.u32 + -8, temp.u32);
	// stfs f0,8(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 8, temp.u32);
	// lfs f13,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// add r29,r7,r29
	ctx.r29.u64 = ctx.r7.u64 + ctx.r29.u64;
	// lfs f0,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// add r30,r30,r8
	ctx.r30.u64 = ctx.r30.u64 + ctx.r8.u64;
	// fsubs f10,f0,f13
	ctx.f10.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// lfs f12,4(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// fadds f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f13.f64));
	// lfs f11,-4(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -4);
	ctx.f11.f64 = double(temp.f32);
	// fmuls f13,f12,f10
	ctx.f13.f64 = double(float(ctx.f12.f64 * ctx.f10.f64));
	// fmuls f12,f12,f0
	ctx.f12.f64 = double(float(ctx.f12.f64 * ctx.f0.f64));
	// fmsubs f0,f11,f0,f13
	ctx.f0.f64 = double(float(std::fma(ctx.f11.f64, ctx.f0.f64, -ctx.f13.f64)));
	// fmadds f13,f11,f10,f12
	ctx.f13.f64 = double(float(std::fma(ctx.f11.f64, ctx.f10.f64, ctx.f12.f64)));
	// stfs f13,-4(r11)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r11.u32 + -4, temp.u32);
	// stfs f0,4(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 4, temp.u32);
	// lfs f13,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// add r29,r7,r29
	ctx.r29.u64 = ctx.r7.u64 + ctx.r29.u64;
	// lfs f0,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// add r30,r30,r8
	ctx.r30.u64 = ctx.r30.u64 + ctx.r8.u64;
	// fsubs f10,f0,f13
	ctx.f10.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// lfs f11,0(r10)
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// lfs f12,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// fadds f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f13.f64));
	// fmuls f13,f11,f10
	ctx.f13.f64 = double(float(ctx.f11.f64 * ctx.f10.f64));
	// fmuls f10,f12,f10
	ctx.f10.f64 = double(float(ctx.f12.f64 * ctx.f10.f64));
	// fmsubs f13,f12,f0,f13
	ctx.f13.f64 = double(float(std::fma(ctx.f12.f64, ctx.f0.f64, -ctx.f13.f64)));
	// fmadds f0,f11,f0,f10
	ctx.f0.f64 = double(float(std::fma(ctx.f11.f64, ctx.f0.f64, ctx.f10.f64)));
	// stfs f0,0(r11)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// stfs f13,0(r10)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// addi r11,r11,16
	ctx.r11.s64 = ctx.r11.s64 + 16;
	// lfs f13,0(r29)
	temp.u32 = REX_LOAD_U32(ctx.r29.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// addi r10,r10,-16
	ctx.r10.s64 = ctx.r10.s64 + -16;
	// lfs f0,0(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// fsubs f10,f0,f13
	ctx.f10.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// lfs f11,0(r27)
	temp.u32 = REX_LOAD_U32(ctx.r27.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fadds f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f13.f64));
	// lfs f12,0(r28)
	temp.u32 = REX_LOAD_U32(ctx.r28.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// fmuls f13,f11,f10
	ctx.f13.f64 = double(float(ctx.f11.f64 * ctx.f10.f64));
	// fmsubs f13,f12,f0,f13
	ctx.f13.f64 = double(float(std::fma(ctx.f12.f64, ctx.f0.f64, -ctx.f13.f64)));
	// fmuls f0,f11,f0
	ctx.f0.f64 = double(float(ctx.f11.f64 * ctx.f0.f64));
	// fmadds f0,f12,f10,f0
	ctx.f0.f64 = double(float(std::fma(ctx.f12.f64, ctx.f10.f64, ctx.f0.f64)));
	// stfs f0,0(r28)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r28.u32 + 0, temp.u32);
	// stfs f13,0(r27)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r27.u32 + 0, temp.u32);
	// bne cr6,0x8295d250
	if (!ctx.cr6.eq) goto loc_8295D250;
loc_8295D35C:
	// cmpw cr6,r25,r24
	ctx.cr6.compare<int32_t>(ctx.r25.s32, ctx.r24.s32, ctx.xer);
	// bge cr6,0x8295d3e8
	if (!ctx.cr6.lt) goto loc_8295D3E8;
	// subf r7,r26,r5
	ctx.r7.u64 = ctx.r5.u64 - ctx.r26.u64;
	// subf r10,r25,r3
	ctx.r10.u64 = ctx.r3.u64 - ctx.r25.u64;
	// rlwinm r7,r7,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// neg r3,r9
	ctx.r3.s64 = static_cast<int64_t>(-ctx.r9.u64);
	// rlwinm r11,r25,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r25.u32 | (ctx.r25.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r26,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r26.u32 | (ctx.r26.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r5,r9,2,0,29
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r7,r6
	ctx.r9.u64 = ctx.r7.u64 + ctx.r6.u64;
	// add r11,r11,r4
	ctx.r11.u64 = ctx.r11.u64 + ctx.r4.u64;
	// add r8,r8,r6
	ctx.r8.u64 = ctx.r8.u64 + ctx.r6.u64;
	// rlwinm r3,r3,2,0,29
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// subf r7,r25,r24
	ctx.r7.u64 = ctx.r24.u64 - ctx.r25.u64;
loc_8295D39C:
	// add r9,r9,r3
	ctx.r9.u64 = ctx.r9.u64 + ctx.r3.u64;
	// lfs f13,0(r10)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// lfs f0,0(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// addi r7,r7,-1
	ctx.r7.s64 = ctx.r7.s64 + -1;
	// cmplwi cr6,r7,0
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 0, ctx.xer);
	// lfs f12,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f12.f64 = double(temp.f32);
	// lfs f11,0(r8)
	temp.u32 = REX_LOAD_U32(ctx.r8.u32 + 0);
	ctx.f11.f64 = double(temp.f32);
	// fsubs f10,f11,f12
	ctx.f10.f64 = double(float(ctx.f11.f64 - ctx.f12.f64));
	// fadds f12,f12,f11
	ctx.f12.f64 = double(float(ctx.f12.f64 + ctx.f11.f64));
	// fmuls f11,f13,f10
	ctx.f11.f64 = double(float(ctx.f13.f64 * ctx.f10.f64));
	// fmuls f10,f0,f10
	ctx.f10.f64 = double(float(ctx.f0.f64 * ctx.f10.f64));
	// fmsubs f0,f0,f12,f11
	ctx.f0.f64 = double(float(std::fma(ctx.f0.f64, ctx.f12.f64, -ctx.f11.f64)));
	// fmadds f13,f13,f12,f10
	ctx.f13.f64 = double(float(std::fma(ctx.f13.f64, ctx.f12.f64, ctx.f10.f64)));
	// stfs f13,0(r11)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// stfs f0,0(r10)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r10.u32 + 0, temp.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// addi r10,r10,-4
	ctx.r10.s64 = ctx.r10.s64 + -4;
	// bne cr6,0x8295d39c
	if (!ctx.cr6.eq) goto loc_8295D39C;
loc_8295D3E8:
	// rlwinm r11,r24,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 2) & 0xFFFFFFFC;
	// lfs f0,0(r6)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r6.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfsx f13,r11,r4
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + ctx.r4.u32);
	ctx.f13.f64 = double(temp.f32);
	// fmuls f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 * ctx.f13.f64));
	// stfsx f0,r11,r4
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r11.u32 + ctx.r4.u32, temp.u32);
	// b 0x824131e8
	__restgprlr_24(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295D400) {
	REX_FUNC_PROLOGUE(sub_8295D400);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// stw r12,-8(r1)
	REX_STORE_U32(ctx.r1.u32 + -8, ctx.r12.u32);
	// stwu r1,-96(r1)
	ea = -96 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r10,r4
	ctx.r10.u64 = ctx.r4.u64;
	// cmpwi cr6,r3,128
	ctx.cr6.compare<int32_t>(ctx.r3.s32, 128, ctx.xer);
	// mr r3,r10
	ctx.r3.u64 = ctx.r10.u64;
	// bne cr6,0x8295d468
	if (!ctx.cr6.eq) goto loc_8295D468;
	// addi r11,r5,-8
	ctx.r11.s64 = ctx.r5.s64 + -8;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r11,r6
	ctx.r9.u64 = ctx.r11.u64 + ctx.r6.u64;
	// mr r4,r9
	ctx.r4.u64 = ctx.r9.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295D430;
	sub_8295C378(ctx, base);
	// addi r11,r5,-32
	ctx.r11.s64 = ctx.r5.s64 + -32;
	// addi r3,r10,128
	ctx.r3.s64 = ctx.r10.s64 + 128;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r6
	ctx.r4.u64 = ctx.r11.u64 + ctx.r6.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295D444;
	sub_8295C798(ctx, base);
	// mr r4,r9
	ctx.r4.u64 = ctx.r9.u64;
	// addi r3,r10,256
	ctx.r3.s64 = ctx.r10.s64 + 256;
	// bl 0x8295c378
	ctx.lr = 0x8295D450;
	sub_8295C378(ctx, base);
	// addi r3,r10,384
	ctx.r3.s64 = ctx.r10.s64 + 384;
	// bl 0x8295c378
	ctx.lr = 0x8295D458;
	sub_8295C378(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
loc_8295D468:
	// addi r11,r5,-16
	ctx.r11.s64 = ctx.r5.s64 + -16;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r6
	ctx.r4.u64 = ctx.r11.u64 + ctx.r6.u64;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D478;
	sub_8295CCD8(ctx, base);
	// addi r3,r10,64
	ctx.r3.s64 = ctx.r10.s64 + 64;
	// bl 0x8295ce60
	ctx.lr = 0x8295D480;
	sub_8295CE60(ctx, base);
	// addi r3,r10,128
	ctx.r3.s64 = ctx.r10.s64 + 128;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D488;
	sub_8295CCD8(ctx, base);
	// addi r3,r10,192
	ctx.r3.s64 = ctx.r10.s64 + 192;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D490;
	sub_8295CCD8(ctx, base);
	// addi r1,r1,96
	ctx.r1.s64 = ctx.r1.s64 + 96;
	// lwz r12,-8(r1)
	ctx.r12.u64 = REX_LOAD_U32(ctx.r1.u32 + -8);
	// mtlr r12
	ctx.lr = ctx.r12.u64;
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_8295D4A0) {
	REX_FUNC_PROLOGUE(sub_8295D4A0);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413180
	ctx.lr = 0x8295D4A8;
	__savegprlr_18(ctx, base);
	// stwu r1,-208(r1)
	ea = -208 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r18,r3
	ctx.r18.u64 = ctx.r3.u64;
	// mr r19,r4
	ctx.r19.u64 = ctx.r4.u64;
	// srawi r31,r18,2
	ctx.xer.ca = (ctx.r18.s32 < 0) & ((ctx.r18.u32 & 0x3) != 0);
	ctx.r31.s64 = ctx.r18.s32 >> 2;
	// mr r29,r5
	ctx.r29.u64 = ctx.r5.u64;
	// mr r28,r6
	ctx.r28.u64 = ctx.r6.u64;
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// ble cr6,0x8295d5ac
	if (!ctx.cr6.gt) goto loc_8295D5AC;
loc_8295D4C8:
	// mr r23,r31
	ctx.r23.u64 = ctx.r31.u64;
	// cmpw cr6,r31,r18
	ctx.cr6.compare<int32_t>(ctx.r31.s32, ctx.r18.s32, ctx.xer);
	// bge cr6,0x8295d57c
	if (!ctx.cr6.lt) goto loc_8295D57C;
loc_8295D4D4:
	// subf r30,r31,r23
	ctx.r30.u64 = ctx.r23.u64 - ctx.r31.u64;
	// cmpw cr6,r30,r18
	ctx.cr6.compare<int32_t>(ctx.r30.s32, ctx.r18.s32, ctx.xer);
	// bge cr6,0x8295d570
	if (!ctx.cr6.lt) goto loc_8295D570;
	// srawi r10,r31,1
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x1) != 0);
	ctx.r10.s64 = ctx.r31.s32 >> 1;
	// rlwinm r11,r23,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 1) & 0xFFFFFFFE;
	// subf r10,r10,r29
	ctx.r10.u64 = ctx.r29.u64 - ctx.r10.u64;
	// subf r9,r31,r29
	ctx.r9.u64 = ctx.r29.u64 - ctx.r31.u64;
	// add r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 + ctx.r30.u64;
	// add r6,r30,r23
	ctx.r6.u64 = ctx.r30.u64 + ctx.r23.u64;
	// rlwinm r8,r10,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r7,r9,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r11,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r30,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r11,r6,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// add r21,r7,r28
	ctx.r21.u64 = ctx.r7.u64 + ctx.r28.u64;
	// add r22,r8,r28
	ctx.r22.u64 = ctx.r8.u64 + ctx.r28.u64;
	// rlwinm r20,r23,2,0,29
	ctx.r20.u64 = __builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r24,r23,4,0,27
	ctx.r24.u64 = __builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 4) & 0xFFFFFFF0;
	// add r27,r9,r19
	ctx.r27.u64 = ctx.r9.u64 + ctx.r19.u64;
	// add r25,r10,r19
	ctx.r25.u64 = ctx.r10.u64 + ctx.r19.u64;
	// add r26,r11,r19
	ctx.r26.u64 = ctx.r11.u64 + ctx.r19.u64;
loc_8295D528:
	// mr r5,r22
	ctx.r5.u64 = ctx.r22.u64;
	// mr r4,r27
	ctx.r4.u64 = ctx.r27.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295D538;
	sub_8295BBB0(ctx, base);
	// mr r5,r21
	ctx.r5.u64 = ctx.r21.u64;
	// mr r4,r26
	ctx.r4.u64 = ctx.r26.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bf38
	ctx.lr = 0x8295D548;
	sub_8295BF38(ctx, base);
	// mr r5,r22
	ctx.r5.u64 = ctx.r22.u64;
	// mr r4,r25
	ctx.r4.u64 = ctx.r25.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295D558;
	sub_8295BBB0(ctx, base);
	// add r30,r20,r30
	ctx.r30.u64 = ctx.r20.u64 + ctx.r30.u64;
	// add r27,r24,r27
	ctx.r27.u64 = ctx.r24.u64 + ctx.r27.u64;
	// add r26,r24,r26
	ctx.r26.u64 = ctx.r24.u64 + ctx.r26.u64;
	// add r25,r24,r25
	ctx.r25.u64 = ctx.r24.u64 + ctx.r25.u64;
	// cmpw cr6,r30,r18
	ctx.cr6.compare<int32_t>(ctx.r30.s32, ctx.r18.s32, ctx.xer);
	// blt cr6,0x8295d528
	if (ctx.cr6.lt) goto loc_8295D528;
loc_8295D570:
	// rlwinm r23,r23,2,0,29
	ctx.r23.u64 = __builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 2) & 0xFFFFFFFC;
	// cmpw cr6,r23,r18
	ctx.cr6.compare<int32_t>(ctx.r23.s32, ctx.r18.s32, ctx.xer);
	// blt cr6,0x8295d4d4
	if (ctx.cr6.lt) goto loc_8295D4D4;
loc_8295D57C:
	// subf r11,r31,r18
	ctx.r11.u64 = ctx.r18.u64 - ctx.r31.u64;
	// srawi r10,r31,1
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x1) != 0);
	ctx.r10.s64 = ctx.r31.s32 >> 1;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// subf r10,r10,r29
	ctx.r10.u64 = ctx.r29.u64 - ctx.r10.u64;
	// add r4,r11,r19
	ctx.r4.u64 = ctx.r11.u64 + ctx.r19.u64;
	// rlwinm r11,r10,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// add r5,r11,r28
	ctx.r5.u64 = ctx.r11.u64 + ctx.r28.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295D5A0;
	sub_8295BBB0(ctx, base);
	// srawi r31,r31,2
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x3) != 0);
	ctx.r31.s64 = ctx.r31.s32 >> 2;
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// bgt cr6,0x8295d4c8
	if (ctx.cr6.gt) goto loc_8295D4C8;
loc_8295D5AC:
	// mr r24,r31
	ctx.r24.u64 = ctx.r31.u64;
	// cmpw cr6,r31,r18
	ctx.cr6.compare<int32_t>(ctx.r31.s32, ctx.r18.s32, ctx.xer);
	// bge cr6,0x8295d7d0
	if (!ctx.cr6.lt) goto loc_8295D7D0;
loc_8295D5B8:
	// subf r25,r31,r24
	ctx.r25.u64 = ctx.r24.u64 - ctx.r31.u64;
	// cmpw cr6,r25,r18
	ctx.cr6.compare<int32_t>(ctx.r25.s32, ctx.r18.s32, ctx.xer);
	// bge cr6,0x8295d7c4
	if (!ctx.cr6.lt) goto loc_8295D7C4;
	// addi r11,r24,48
	ctx.r11.s64 = ctx.r24.s64 + 48;
	// srawi r9,r31,1
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x1) != 0);
	ctx.r9.s64 = ctx.r31.s32 >> 1;
	// rlwinm r10,r11,1,0,30
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// add r11,r25,r24
	ctx.r11.u64 = ctx.r25.u64 + ctx.r24.u64;
	// subf r8,r31,r29
	ctx.r8.u64 = ctx.r29.u64 - ctx.r31.u64;
	// subf r9,r9,r29
	ctx.r9.u64 = ctx.r29.u64 - ctx.r9.u64;
	// addi r6,r25,96
	ctx.r6.s64 = ctx.r25.s64 + 96;
	// add r10,r10,r25
	ctx.r10.u64 = ctx.r10.u64 + ctx.r25.u64;
	// addi r11,r11,96
	ctx.r11.s64 = ctx.r11.s64 + 96;
	// rlwinm r7,r8,2,0,29
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r8,r9,2,0,29
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r6,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r10,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r20,r7,r28
	ctx.r20.u64 = ctx.r7.u64 + ctx.r28.u64;
	// add r22,r8,r28
	ctx.r22.u64 = ctx.r8.u64 + ctx.r28.u64;
	// rlwinm r21,r24,2,0,29
	ctx.r21.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r23,r24,4,0,27
	ctx.r23.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 4) & 0xFFFFFFF0;
	// add r27,r9,r19
	ctx.r27.u64 = ctx.r9.u64 + ctx.r19.u64;
	// add r26,r10,r19
	ctx.r26.u64 = ctx.r10.u64 + ctx.r19.u64;
	// add r30,r11,r19
	ctx.r30.u64 = ctx.r11.u64 + ctx.r19.u64;
loc_8295D618:
	// addi r4,r27,-384
	ctx.r4.s64 = ctx.r27.s64 + -384;
	// mr r5,r22
	ctx.r5.u64 = ctx.r22.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295D628;
	sub_8295BBB0(ctx, base);
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// mr r3,r4
	ctx.r3.u64 = ctx.r4.u64;
	// bne cr6,0x8295d674
	if (!ctx.cr6.eq) goto loc_8295D674;
	// addi r11,r29,-8
	ctx.r11.s64 = ctx.r29.s64 + -8;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r11,r28
	ctx.r10.u64 = ctx.r11.u64 + ctx.r28.u64;
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295D648;
	sub_8295C378(ctx, base);
	// addi r11,r29,-32
	ctx.r11.s64 = ctx.r29.s64 + -32;
	// addi r3,r27,-256
	ctx.r3.s64 = ctx.r27.s64 + -256;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r28
	ctx.r4.u64 = ctx.r11.u64 + ctx.r28.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295D65C;
	sub_8295C798(ctx, base);
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// addi r3,r27,-128
	ctx.r3.s64 = ctx.r27.s64 + -128;
	// bl 0x8295c378
	ctx.lr = 0x8295D668;
	sub_8295C378(ctx, base);
	// mr r3,r27
	ctx.r3.u64 = ctx.r27.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295D670;
	sub_8295C378(ctx, base);
	// b 0x8295d69c
	goto loc_8295D69C;
loc_8295D674:
	// addi r11,r29,-16
	ctx.r11.s64 = ctx.r29.s64 + -16;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r28
	ctx.r4.u64 = ctx.r11.u64 + ctx.r28.u64;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D684;
	sub_8295CCD8(ctx, base);
	// addi r3,r27,-320
	ctx.r3.s64 = ctx.r27.s64 + -320;
	// bl 0x8295ce60
	ctx.lr = 0x8295D68C;
	sub_8295CE60(ctx, base);
	// addi r3,r27,-256
	ctx.r3.s64 = ctx.r27.s64 + -256;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D694;
	sub_8295CCD8(ctx, base);
	// addi r3,r27,-192
	ctx.r3.s64 = ctx.r27.s64 + -192;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D69C;
	sub_8295CCD8(ctx, base);
loc_8295D69C:
	// addi r4,r30,-384
	ctx.r4.s64 = ctx.r30.s64 + -384;
	// mr r5,r20
	ctx.r5.u64 = ctx.r20.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bf38
	ctx.lr = 0x8295D6AC;
	sub_8295BF38(ctx, base);
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// mr r3,r4
	ctx.r3.u64 = ctx.r4.u64;
	// bne cr6,0x8295d700
	if (!ctx.cr6.eq) goto loc_8295D700;
	// addi r11,r29,-8
	ctx.r11.s64 = ctx.r29.s64 + -8;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r11,r28
	ctx.r10.u64 = ctx.r11.u64 + ctx.r28.u64;
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295D6CC;
	sub_8295C378(ctx, base);
	// addi r11,r29,-32
	ctx.r11.s64 = ctx.r29.s64 + -32;
	// addi r3,r30,-256
	ctx.r3.s64 = ctx.r30.s64 + -256;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r11,r28
	ctx.r9.u64 = ctx.r11.u64 + ctx.r28.u64;
	// mr r4,r9
	ctx.r4.u64 = ctx.r9.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295D6E4;
	sub_8295C798(ctx, base);
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// addi r3,r30,-128
	ctx.r3.s64 = ctx.r30.s64 + -128;
	// bl 0x8295c378
	ctx.lr = 0x8295D6F0;
	sub_8295C378(ctx, base);
	// mr r4,r9
	ctx.r4.u64 = ctx.r9.u64;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295D6FC;
	sub_8295C798(ctx, base);
	// b 0x8295d728
	goto loc_8295D728;
loc_8295D700:
	// addi r11,r29,-16
	ctx.r11.s64 = ctx.r29.s64 + -16;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r28
	ctx.r4.u64 = ctx.r11.u64 + ctx.r28.u64;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D710;
	sub_8295CCD8(ctx, base);
	// addi r3,r30,-320
	ctx.r3.s64 = ctx.r30.s64 + -320;
	// bl 0x8295ce60
	ctx.lr = 0x8295D718;
	sub_8295CE60(ctx, base);
	// addi r3,r30,-256
	ctx.r3.s64 = ctx.r30.s64 + -256;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D720;
	sub_8295CCD8(ctx, base);
	// addi r3,r30,-192
	ctx.r3.s64 = ctx.r30.s64 + -192;
	// bl 0x8295ce60
	ctx.lr = 0x8295D728;
	sub_8295CE60(ctx, base);
loc_8295D728:
	// addi r4,r26,-384
	ctx.r4.s64 = ctx.r26.s64 + -384;
	// mr r5,r22
	ctx.r5.u64 = ctx.r22.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295D738;
	sub_8295BBB0(ctx, base);
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// mr r3,r4
	ctx.r3.u64 = ctx.r4.u64;
	// bne cr6,0x8295d784
	if (!ctx.cr6.eq) goto loc_8295D784;
	// addi r11,r29,-8
	ctx.r11.s64 = ctx.r29.s64 + -8;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r11,r28
	ctx.r10.u64 = ctx.r11.u64 + ctx.r28.u64;
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295D758;
	sub_8295C378(ctx, base);
	// addi r11,r29,-32
	ctx.r11.s64 = ctx.r29.s64 + -32;
	// addi r3,r26,-256
	ctx.r3.s64 = ctx.r26.s64 + -256;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r28
	ctx.r4.u64 = ctx.r11.u64 + ctx.r28.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295D76C;
	sub_8295C798(ctx, base);
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// addi r3,r26,-128
	ctx.r3.s64 = ctx.r26.s64 + -128;
	// bl 0x8295c378
	ctx.lr = 0x8295D778;
	sub_8295C378(ctx, base);
	// mr r3,r26
	ctx.r3.u64 = ctx.r26.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295D780;
	sub_8295C378(ctx, base);
	// b 0x8295d7ac
	goto loc_8295D7AC;
loc_8295D784:
	// addi r11,r29,-16
	ctx.r11.s64 = ctx.r29.s64 + -16;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r28
	ctx.r4.u64 = ctx.r11.u64 + ctx.r28.u64;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D794;
	sub_8295CCD8(ctx, base);
	// addi r3,r26,-320
	ctx.r3.s64 = ctx.r26.s64 + -320;
	// bl 0x8295ce60
	ctx.lr = 0x8295D79C;
	sub_8295CE60(ctx, base);
	// addi r3,r26,-256
	ctx.r3.s64 = ctx.r26.s64 + -256;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D7A4;
	sub_8295CCD8(ctx, base);
	// addi r3,r26,-192
	ctx.r3.s64 = ctx.r26.s64 + -192;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D7AC;
	sub_8295CCD8(ctx, base);
loc_8295D7AC:
	// add r25,r21,r25
	ctx.r25.u64 = ctx.r21.u64 + ctx.r25.u64;
	// add r27,r27,r23
	ctx.r27.u64 = ctx.r27.u64 + ctx.r23.u64;
	// add r30,r30,r23
	ctx.r30.u64 = ctx.r30.u64 + ctx.r23.u64;
	// add r26,r26,r23
	ctx.r26.u64 = ctx.r26.u64 + ctx.r23.u64;
	// cmpw cr6,r25,r18
	ctx.cr6.compare<int32_t>(ctx.r25.s32, ctx.r18.s32, ctx.xer);
	// blt cr6,0x8295d618
	if (ctx.cr6.lt) goto loc_8295D618;
loc_8295D7C4:
	// rlwinm r24,r24,2,0,29
	ctx.r24.u64 = __builtin_rotateleft64(ctx.r24.u32 | (ctx.r24.u64 << 32), 2) & 0xFFFFFFFC;
	// cmpw cr6,r24,r18
	ctx.cr6.compare<int32_t>(ctx.r24.s32, ctx.r18.s32, ctx.xer);
	// blt cr6,0x8295d5b8
	if (ctx.cr6.lt) goto loc_8295D5B8;
loc_8295D7D0:
	// subf r11,r31,r18
	ctx.r11.u64 = ctx.r18.u64 - ctx.r31.u64;
	// srawi r10,r31,1
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x1) != 0);
	ctx.r10.s64 = ctx.r31.s32 >> 1;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// add r30,r11,r19
	ctx.r30.u64 = ctx.r11.u64 + ctx.r19.u64;
	// subf r11,r10,r29
	ctx.r11.u64 = ctx.r29.u64 - ctx.r10.u64;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r5,r11,r28
	ctx.r5.u64 = ctx.r11.u64 + ctx.r28.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295D7F8;
	sub_8295BBB0(ctx, base);
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// bne cr6,0x8295d848
	if (!ctx.cr6.eq) goto loc_8295D848;
	// addi r11,r29,-8
	ctx.r11.s64 = ctx.r29.s64 + -8;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r11,r28
	ctx.r10.u64 = ctx.r11.u64 + ctx.r28.u64;
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295D818;
	sub_8295C378(ctx, base);
	// addi r11,r29,-32
	ctx.r11.s64 = ctx.r29.s64 + -32;
	// addi r3,r30,128
	ctx.r3.s64 = ctx.r30.s64 + 128;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r28
	ctx.r4.u64 = ctx.r11.u64 + ctx.r28.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295D82C;
	sub_8295C798(ctx, base);
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// addi r3,r30,256
	ctx.r3.s64 = ctx.r30.s64 + 256;
	// bl 0x8295c378
	ctx.lr = 0x8295D838;
	sub_8295C378(ctx, base);
	// addi r3,r30,384
	ctx.r3.s64 = ctx.r30.s64 + 384;
	// bl 0x8295c378
	ctx.lr = 0x8295D840;
	sub_8295C378(ctx, base);
	// addi r1,r1,208
	ctx.r1.s64 = ctx.r1.s64 + 208;
	// b 0x824131d0
	__restgprlr_18(ctx, base);
	return;
loc_8295D848:
	// addi r11,r29,-16
	ctx.r11.s64 = ctx.r29.s64 + -16;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r28
	ctx.r4.u64 = ctx.r11.u64 + ctx.r28.u64;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D858;
	sub_8295CCD8(ctx, base);
	// addi r3,r30,64
	ctx.r3.s64 = ctx.r30.s64 + 64;
	// bl 0x8295ce60
	ctx.lr = 0x8295D860;
	sub_8295CE60(ctx, base);
	// addi r3,r30,128
	ctx.r3.s64 = ctx.r30.s64 + 128;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D868;
	sub_8295CCD8(ctx, base);
	// addi r3,r30,192
	ctx.r3.s64 = ctx.r30.s64 + 192;
	// bl 0x8295ccd8
	ctx.lr = 0x8295D870;
	sub_8295CCD8(ctx, base);
	// addi r1,r1,208
	ctx.r1.s64 = ctx.r1.s64 + 208;
	// b 0x824131d0
	__restgprlr_18(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295D878) {
	REX_FUNC_PROLOGUE(sub_8295D878);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413188
	ctx.lr = 0x8295D880;
	__savegprlr_20(ctx, base);
	// stwu r1,-192(r1)
	ea = -192 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// srawi r22,r3,1
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0x1) != 0);
	ctx.r22.s64 = ctx.r3.s32 >> 1;
	// srawi r31,r3,2
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0x3) != 0);
	ctx.r31.s64 = ctx.r3.s32 >> 2;
	// mr r20,r4
	ctx.r20.u64 = ctx.r4.u64;
	// mr r27,r5
	ctx.r27.u64 = ctx.r5.u64;
	// mr r26,r6
	ctx.r26.u64 = ctx.r6.u64;
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// ble cr6,0x8295d994
	if (!ctx.cr6.gt) goto loc_8295D994;
loc_8295D8A0:
	// mr r23,r31
	ctx.r23.u64 = ctx.r31.u64;
	// cmpw cr6,r31,r22
	ctx.cr6.compare<int32_t>(ctx.r31.s32, ctx.r22.s32, ctx.xer);
	// bge cr6,0x8295d988
	if (!ctx.cr6.lt) goto loc_8295D988;
loc_8295D8AC:
	// subf r30,r31,r23
	ctx.r30.u64 = ctx.r23.u64 - ctx.r31.u64;
	// cmpw cr6,r30,r22
	ctx.cr6.compare<int32_t>(ctx.r30.s32, ctx.r22.s32, ctx.xer);
	// bge cr6,0x8295d918
	if (!ctx.cr6.lt) goto loc_8295D918;
	// srawi r9,r31,1
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x1) != 0);
	ctx.r9.s64 = ctx.r31.s32 >> 1;
	// add r11,r30,r22
	ctx.r11.u64 = ctx.r30.u64 + ctx.r22.u64;
	// subf r9,r9,r27
	ctx.r9.u64 = ctx.r27.u64 - ctx.r9.u64;
	// rlwinm r10,r30,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r9,r9,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r21,r23,1,0,30
	ctx.r21.u64 = __builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 1) & 0xFFFFFFFE;
	// add r25,r9,r26
	ctx.r25.u64 = ctx.r9.u64 + ctx.r26.u64;
	// rlwinm r24,r23,3,0,28
	ctx.r24.u64 = __builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 3) & 0xFFFFFFF8;
	// add r29,r10,r20
	ctx.r29.u64 = ctx.r10.u64 + ctx.r20.u64;
	// add r28,r11,r20
	ctx.r28.u64 = ctx.r11.u64 + ctx.r20.u64;
loc_8295D8E4:
	// mr r5,r25
	ctx.r5.u64 = ctx.r25.u64;
	// mr r4,r29
	ctx.r4.u64 = ctx.r29.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295D8F4;
	sub_8295BBB0(ctx, base);
	// mr r5,r25
	ctx.r5.u64 = ctx.r25.u64;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295D904;
	sub_8295BBB0(ctx, base);
	// add r30,r21,r30
	ctx.r30.u64 = ctx.r21.u64 + ctx.r30.u64;
	// add r29,r24,r29
	ctx.r29.u64 = ctx.r24.u64 + ctx.r29.u64;
	// add r28,r24,r28
	ctx.r28.u64 = ctx.r24.u64 + ctx.r28.u64;
	// cmpw cr6,r30,r22
	ctx.cr6.compare<int32_t>(ctx.r30.s32, ctx.r22.s32, ctx.xer);
	// blt cr6,0x8295d8e4
	if (ctx.cr6.lt) goto loc_8295D8E4;
loc_8295D918:
	// rlwinm r11,r23,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 1) & 0xFFFFFFFE;
	// subf r30,r31,r11
	ctx.r30.u64 = ctx.r11.u64 - ctx.r31.u64;
	// cmpw cr6,r30,r22
	ctx.cr6.compare<int32_t>(ctx.r30.s32, ctx.r22.s32, ctx.xer);
	// bge cr6,0x8295d97c
	if (!ctx.cr6.lt) goto loc_8295D97C;
	// subf r11,r31,r27
	ctx.r11.u64 = ctx.r27.u64 - ctx.r31.u64;
	// add r8,r30,r22
	ctx.r8.u64 = ctx.r30.u64 + ctx.r22.u64;
	// rlwinm r9,r11,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r30,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r11,r8,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// add r5,r9,r26
	ctx.r5.u64 = ctx.r9.u64 + ctx.r26.u64;
	// rlwinm r24,r23,2,0,29
	ctx.r24.u64 = __builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r25,r23,4,0,27
	ctx.r25.u64 = __builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 4) & 0xFFFFFFF0;
	// add r29,r10,r20
	ctx.r29.u64 = ctx.r10.u64 + ctx.r20.u64;
	// add r28,r11,r20
	ctx.r28.u64 = ctx.r11.u64 + ctx.r20.u64;
loc_8295D950:
	// mr r4,r29
	ctx.r4.u64 = ctx.r29.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bf38
	ctx.lr = 0x8295D95C;
	sub_8295BF38(ctx, base);
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bf38
	ctx.lr = 0x8295D968;
	sub_8295BF38(ctx, base);
	// add r30,r24,r30
	ctx.r30.u64 = ctx.r24.u64 + ctx.r30.u64;
	// add r29,r25,r29
	ctx.r29.u64 = ctx.r25.u64 + ctx.r29.u64;
	// add r28,r25,r28
	ctx.r28.u64 = ctx.r25.u64 + ctx.r28.u64;
	// cmpw cr6,r30,r22
	ctx.cr6.compare<int32_t>(ctx.r30.s32, ctx.r22.s32, ctx.xer);
	// blt cr6,0x8295d950
	if (ctx.cr6.lt) goto loc_8295D950;
loc_8295D97C:
	// rlwinm r23,r23,2,0,29
	ctx.r23.u64 = __builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 2) & 0xFFFFFFFC;
	// cmpw cr6,r23,r22
	ctx.cr6.compare<int32_t>(ctx.r23.s32, ctx.r22.s32, ctx.xer);
	// blt cr6,0x8295d8ac
	if (ctx.cr6.lt) goto loc_8295D8AC;
loc_8295D988:
	// srawi r31,r31,2
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x3) != 0);
	ctx.r31.s64 = ctx.r31.s32 >> 2;
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// bgt cr6,0x8295d8a0
	if (ctx.cr6.gt) goto loc_8295D8A0;
loc_8295D994:
	// mr r21,r31
	ctx.r21.u64 = ctx.r31.u64;
	// cmpw cr6,r31,r22
	ctx.cr6.compare<int32_t>(ctx.r31.s32, ctx.r22.s32, ctx.xer);
	// bge cr6,0x8295dc6c
	if (!ctx.cr6.lt) goto loc_8295DC6C;
loc_8295D9A0:
	// subf r28,r31,r21
	ctx.r28.u64 = ctx.r21.u64 - ctx.r31.u64;
	// cmpw cr6,r28,r22
	ctx.cr6.compare<int32_t>(ctx.r28.s32, ctx.r22.s32, ctx.xer);
	// bge cr6,0x8295dafc
	if (!ctx.cr6.lt) goto loc_8295DAFC;
	// srawi r10,r31,1
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x1) != 0);
	ctx.r10.s64 = ctx.r31.s32 >> 1;
	// add r11,r28,r22
	ctx.r11.u64 = ctx.r28.u64 + ctx.r22.u64;
	// subf r10,r10,r27
	ctx.r10.u64 = ctx.r27.u64 - ctx.r10.u64;
	// addi r8,r28,96
	ctx.r8.s64 = ctx.r28.s64 + 96;
	// addi r11,r11,96
	ctx.r11.s64 = ctx.r11.s64 + 96;
	// rlwinm r9,r10,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r8,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r24,r9,r26
	ctx.r24.u64 = ctx.r9.u64 + ctx.r26.u64;
	// rlwinm r23,r21,1,0,30
	ctx.r23.u64 = __builtin_rotateleft64(ctx.r21.u32 | (ctx.r21.u64 << 32), 1) & 0xFFFFFFFE;
	// rlwinm r25,r21,3,0,28
	ctx.r25.u64 = __builtin_rotateleft64(ctx.r21.u32 | (ctx.r21.u64 << 32), 3) & 0xFFFFFFF8;
	// add r30,r10,r20
	ctx.r30.u64 = ctx.r10.u64 + ctx.r20.u64;
	// add r29,r11,r20
	ctx.r29.u64 = ctx.r11.u64 + ctx.r20.u64;
loc_8295D9E0:
	// addi r4,r30,-384
	ctx.r4.s64 = ctx.r30.s64 + -384;
	// mr r5,r24
	ctx.r5.u64 = ctx.r24.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295D9F0;
	sub_8295BBB0(ctx, base);
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// mr r3,r4
	ctx.r3.u64 = ctx.r4.u64;
	// bne cr6,0x8295da3c
	if (!ctx.cr6.eq) goto loc_8295DA3C;
	// addi r11,r27,-8
	ctx.r11.s64 = ctx.r27.s64 + -8;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r11,r26
	ctx.r10.u64 = ctx.r11.u64 + ctx.r26.u64;
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295DA10;
	sub_8295C378(ctx, base);
	// addi r11,r27,-32
	ctx.r11.s64 = ctx.r27.s64 + -32;
	// addi r3,r30,-256
	ctx.r3.s64 = ctx.r30.s64 + -256;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r26
	ctx.r4.u64 = ctx.r11.u64 + ctx.r26.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295DA24;
	sub_8295C798(ctx, base);
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// addi r3,r30,-128
	ctx.r3.s64 = ctx.r30.s64 + -128;
	// bl 0x8295c378
	ctx.lr = 0x8295DA30;
	sub_8295C378(ctx, base);
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295DA38;
	sub_8295C378(ctx, base);
	// b 0x8295da64
	goto loc_8295DA64;
loc_8295DA3C:
	// addi r11,r27,-16
	ctx.r11.s64 = ctx.r27.s64 + -16;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r26
	ctx.r4.u64 = ctx.r11.u64 + ctx.r26.u64;
	// bl 0x8295ccd8
	ctx.lr = 0x8295DA4C;
	sub_8295CCD8(ctx, base);
	// addi r3,r30,-320
	ctx.r3.s64 = ctx.r30.s64 + -320;
	// bl 0x8295ce60
	ctx.lr = 0x8295DA54;
	sub_8295CE60(ctx, base);
	// addi r3,r30,-256
	ctx.r3.s64 = ctx.r30.s64 + -256;
	// bl 0x8295ccd8
	ctx.lr = 0x8295DA5C;
	sub_8295CCD8(ctx, base);
	// addi r3,r30,-192
	ctx.r3.s64 = ctx.r30.s64 + -192;
	// bl 0x8295ccd8
	ctx.lr = 0x8295DA64;
	sub_8295CCD8(ctx, base);
loc_8295DA64:
	// addi r4,r29,-384
	ctx.r4.s64 = ctx.r29.s64 + -384;
	// mr r5,r24
	ctx.r5.u64 = ctx.r24.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295DA74;
	sub_8295BBB0(ctx, base);
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// mr r3,r4
	ctx.r3.u64 = ctx.r4.u64;
	// bne cr6,0x8295dac0
	if (!ctx.cr6.eq) goto loc_8295DAC0;
	// addi r11,r27,-8
	ctx.r11.s64 = ctx.r27.s64 + -8;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r11,r26
	ctx.r10.u64 = ctx.r11.u64 + ctx.r26.u64;
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295DA94;
	sub_8295C378(ctx, base);
	// addi r11,r27,-32
	ctx.r11.s64 = ctx.r27.s64 + -32;
	// addi r3,r29,-256
	ctx.r3.s64 = ctx.r29.s64 + -256;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r26
	ctx.r4.u64 = ctx.r11.u64 + ctx.r26.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295DAA8;
	sub_8295C798(ctx, base);
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// addi r3,r29,-128
	ctx.r3.s64 = ctx.r29.s64 + -128;
	// bl 0x8295c378
	ctx.lr = 0x8295DAB4;
	sub_8295C378(ctx, base);
	// mr r3,r29
	ctx.r3.u64 = ctx.r29.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295DABC;
	sub_8295C378(ctx, base);
	// b 0x8295dae8
	goto loc_8295DAE8;
loc_8295DAC0:
	// addi r11,r27,-16
	ctx.r11.s64 = ctx.r27.s64 + -16;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r26
	ctx.r4.u64 = ctx.r11.u64 + ctx.r26.u64;
	// bl 0x8295ccd8
	ctx.lr = 0x8295DAD0;
	sub_8295CCD8(ctx, base);
	// addi r3,r29,-320
	ctx.r3.s64 = ctx.r29.s64 + -320;
	// bl 0x8295ce60
	ctx.lr = 0x8295DAD8;
	sub_8295CE60(ctx, base);
	// addi r3,r29,-256
	ctx.r3.s64 = ctx.r29.s64 + -256;
	// bl 0x8295ccd8
	ctx.lr = 0x8295DAE0;
	sub_8295CCD8(ctx, base);
	// addi r3,r29,-192
	ctx.r3.s64 = ctx.r29.s64 + -192;
	// bl 0x8295ccd8
	ctx.lr = 0x8295DAE8;
	sub_8295CCD8(ctx, base);
loc_8295DAE8:
	// add r28,r23,r28
	ctx.r28.u64 = ctx.r23.u64 + ctx.r28.u64;
	// add r30,r30,r25
	ctx.r30.u64 = ctx.r30.u64 + ctx.r25.u64;
	// add r29,r29,r25
	ctx.r29.u64 = ctx.r29.u64 + ctx.r25.u64;
	// cmpw cr6,r28,r22
	ctx.cr6.compare<int32_t>(ctx.r28.s32, ctx.r22.s32, ctx.xer);
	// blt cr6,0x8295d9e0
	if (ctx.cr6.lt) goto loc_8295D9E0;
loc_8295DAFC:
	// rlwinm r11,r21,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r21.u32 | (ctx.r21.u64 << 32), 1) & 0xFFFFFFFE;
	// subf r28,r31,r11
	ctx.r28.u64 = ctx.r11.u64 - ctx.r31.u64;
	// cmpw cr6,r28,r22
	ctx.cr6.compare<int32_t>(ctx.r28.s32, ctx.r22.s32, ctx.xer);
	// bge cr6,0x8295dc60
	if (!ctx.cr6.lt) goto loc_8295DC60;
	// add r11,r28,r22
	ctx.r11.u64 = ctx.r28.u64 + ctx.r22.u64;
	// subf r10,r31,r27
	ctx.r10.u64 = ctx.r27.u64 - ctx.r31.u64;
	// addi r8,r28,96
	ctx.r8.s64 = ctx.r28.s64 + 96;
	// addi r11,r11,96
	ctx.r11.s64 = ctx.r11.s64 + 96;
	// rlwinm r9,r10,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r8,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r5,r9,r26
	ctx.r5.u64 = ctx.r9.u64 + ctx.r26.u64;
	// rlwinm r24,r21,2,0,29
	ctx.r24.u64 = __builtin_rotateleft64(ctx.r21.u32 | (ctx.r21.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r25,r21,4,0,27
	ctx.r25.u64 = __builtin_rotateleft64(ctx.r21.u32 | (ctx.r21.u64 << 32), 4) & 0xFFFFFFF0;
	// add r30,r10,r20
	ctx.r30.u64 = ctx.r10.u64 + ctx.r20.u64;
	// add r29,r11,r20
	ctx.r29.u64 = ctx.r11.u64 + ctx.r20.u64;
loc_8295DB3C:
	// addi r4,r30,-384
	ctx.r4.s64 = ctx.r30.s64 + -384;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bf38
	ctx.lr = 0x8295DB48;
	sub_8295BF38(ctx, base);
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// mr r3,r4
	ctx.r3.u64 = ctx.r4.u64;
	// bne cr6,0x8295db9c
	if (!ctx.cr6.eq) goto loc_8295DB9C;
	// addi r11,r27,-8
	ctx.r11.s64 = ctx.r27.s64 + -8;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r11,r26
	ctx.r10.u64 = ctx.r11.u64 + ctx.r26.u64;
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295DB68;
	sub_8295C378(ctx, base);
	// addi r11,r27,-32
	ctx.r11.s64 = ctx.r27.s64 + -32;
	// addi r3,r30,-256
	ctx.r3.s64 = ctx.r30.s64 + -256;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r11,r26
	ctx.r9.u64 = ctx.r11.u64 + ctx.r26.u64;
	// mr r4,r9
	ctx.r4.u64 = ctx.r9.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295DB80;
	sub_8295C798(ctx, base);
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// addi r3,r30,-128
	ctx.r3.s64 = ctx.r30.s64 + -128;
	// bl 0x8295c378
	ctx.lr = 0x8295DB8C;
	sub_8295C378(ctx, base);
	// mr r4,r9
	ctx.r4.u64 = ctx.r9.u64;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295DB98;
	sub_8295C798(ctx, base);
	// b 0x8295dbc4
	goto loc_8295DBC4;
loc_8295DB9C:
	// addi r11,r27,-16
	ctx.r11.s64 = ctx.r27.s64 + -16;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r26
	ctx.r4.u64 = ctx.r11.u64 + ctx.r26.u64;
	// bl 0x8295ccd8
	ctx.lr = 0x8295DBAC;
	sub_8295CCD8(ctx, base);
	// addi r3,r30,-320
	ctx.r3.s64 = ctx.r30.s64 + -320;
	// bl 0x8295ce60
	ctx.lr = 0x8295DBB4;
	sub_8295CE60(ctx, base);
	// addi r3,r30,-256
	ctx.r3.s64 = ctx.r30.s64 + -256;
	// bl 0x8295ccd8
	ctx.lr = 0x8295DBBC;
	sub_8295CCD8(ctx, base);
	// addi r3,r30,-192
	ctx.r3.s64 = ctx.r30.s64 + -192;
	// bl 0x8295ce60
	ctx.lr = 0x8295DBC4;
	sub_8295CE60(ctx, base);
loc_8295DBC4:
	// addi r4,r29,-384
	ctx.r4.s64 = ctx.r29.s64 + -384;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295bf38
	ctx.lr = 0x8295DBD0;
	sub_8295BF38(ctx, base);
	// cmpwi cr6,r31,128
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 128, ctx.xer);
	// mr r3,r4
	ctx.r3.u64 = ctx.r4.u64;
	// bne cr6,0x8295dc24
	if (!ctx.cr6.eq) goto loc_8295DC24;
	// addi r11,r27,-8
	ctx.r11.s64 = ctx.r27.s64 + -8;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r10,r11,r26
	ctx.r10.u64 = ctx.r11.u64 + ctx.r26.u64;
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295DBF0;
	sub_8295C378(ctx, base);
	// addi r11,r27,-32
	ctx.r11.s64 = ctx.r27.s64 + -32;
	// addi r3,r29,-256
	ctx.r3.s64 = ctx.r29.s64 + -256;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r11,r26
	ctx.r9.u64 = ctx.r11.u64 + ctx.r26.u64;
	// mr r4,r9
	ctx.r4.u64 = ctx.r9.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295DC08;
	sub_8295C798(ctx, base);
	// mr r4,r10
	ctx.r4.u64 = ctx.r10.u64;
	// addi r3,r29,-128
	ctx.r3.s64 = ctx.r29.s64 + -128;
	// bl 0x8295c378
	ctx.lr = 0x8295DC14;
	sub_8295C378(ctx, base);
	// mr r4,r9
	ctx.r4.u64 = ctx.r9.u64;
	// mr r3,r29
	ctx.r3.u64 = ctx.r29.u64;
	// bl 0x8295c798
	ctx.lr = 0x8295DC20;
	sub_8295C798(ctx, base);
	// b 0x8295dc4c
	goto loc_8295DC4C;
loc_8295DC24:
	// addi r11,r27,-16
	ctx.r11.s64 = ctx.r27.s64 + -16;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r26
	ctx.r4.u64 = ctx.r11.u64 + ctx.r26.u64;
	// bl 0x8295ccd8
	ctx.lr = 0x8295DC34;
	sub_8295CCD8(ctx, base);
	// addi r3,r29,-320
	ctx.r3.s64 = ctx.r29.s64 + -320;
	// bl 0x8295ce60
	ctx.lr = 0x8295DC3C;
	sub_8295CE60(ctx, base);
	// addi r3,r29,-256
	ctx.r3.s64 = ctx.r29.s64 + -256;
	// bl 0x8295ccd8
	ctx.lr = 0x8295DC44;
	sub_8295CCD8(ctx, base);
	// addi r3,r29,-192
	ctx.r3.s64 = ctx.r29.s64 + -192;
	// bl 0x8295ce60
	ctx.lr = 0x8295DC4C;
	sub_8295CE60(ctx, base);
loc_8295DC4C:
	// add r28,r24,r28
	ctx.r28.u64 = ctx.r24.u64 + ctx.r28.u64;
	// add r30,r25,r30
	ctx.r30.u64 = ctx.r25.u64 + ctx.r30.u64;
	// add r29,r25,r29
	ctx.r29.u64 = ctx.r25.u64 + ctx.r29.u64;
	// cmpw cr6,r28,r22
	ctx.cr6.compare<int32_t>(ctx.r28.s32, ctx.r22.s32, ctx.xer);
	// blt cr6,0x8295db3c
	if (ctx.cr6.lt) goto loc_8295DB3C;
loc_8295DC60:
	// rlwinm r21,r21,2,0,29
	ctx.r21.u64 = __builtin_rotateleft64(ctx.r21.u32 | (ctx.r21.u64 << 32), 2) & 0xFFFFFFFC;
	// cmpw cr6,r21,r22
	ctx.cr6.compare<int32_t>(ctx.r21.s32, ctx.r22.s32, ctx.xer);
	// blt cr6,0x8295d9a0
	if (ctx.cr6.lt) goto loc_8295D9A0;
loc_8295DC6C:
	// addi r1,r1,192
	ctx.r1.s64 = ctx.r1.s64 + 192;
	// b 0x824131d8
	__restgprlr_20(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295DC78) {
	REX_FUNC_PROLOGUE(sub_8295DC78);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a4
	ctx.lr = 0x8295DC80;
	__savegprlr_27(ctx, base);
	// stwu r1,-128(r1)
	ea = -128 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r27,r3
	ctx.r27.u64 = ctx.r3.u64;
	// mr r29,r5
	ctx.r29.u64 = ctx.r5.u64;
	// srawi r31,r27,2
	ctx.xer.ca = (ctx.r27.s32 < 0) & ((ctx.r27.u32 & 0x3) != 0);
	ctx.r31.s64 = ctx.r27.s32 >> 2;
	// mr r28,r6
	ctx.r28.u64 = ctx.r6.u64;
	// rlwinm r11,r31,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 1) & 0xFFFFFFFE;
	// mr r30,r4
	ctx.r30.u64 = ctx.r4.u64;
	// subf r11,r11,r29
	ctx.r11.u64 = ctx.r29.u64 - ctx.r11.u64;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r5,r11,r28
	ctx.r5.u64 = ctx.r11.u64 + ctx.r28.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295DCAC;
	sub_8295BBB0(ctx, base);
	// cmpwi cr6,r27,512
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 512, ctx.xer);
	// ble cr6,0x8295dd38
	if (!ctx.cr6.gt) goto loc_8295DD38;
loc_8295DCB4:
	// mr r6,r28
	ctx.r6.u64 = ctx.r28.u64;
	// mr r5,r29
	ctx.r5.u64 = ctx.r29.u64;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295dc78
	ctx.lr = 0x8295DCC8;
	sub_8295DC78(ctx, base);
	// rlwinm r11,r31,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r6,r28
	ctx.r6.u64 = ctx.r28.u64;
	// mr r5,r29
	ctx.r5.u64 = ctx.r29.u64;
	// add r4,r11,r30
	ctx.r4.u64 = ctx.r11.u64 + ctx.r30.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295dd58
	ctx.lr = 0x8295DCE0;
	sub_8295DD58(ctx, base);
	// rlwinm r11,r31,3,0,28
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 3) & 0xFFFFFFF8;
	// mr r6,r28
	ctx.r6.u64 = ctx.r28.u64;
	// mr r5,r29
	ctx.r5.u64 = ctx.r29.u64;
	// add r4,r11,r30
	ctx.r4.u64 = ctx.r11.u64 + ctx.r30.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295dc78
	ctx.lr = 0x8295DCF8;
	sub_8295DC78(ctx, base);
	// mr r11,r31
	ctx.r11.u64 = ctx.r31.u64;
	// mr r27,r31
	ctx.r27.u64 = ctx.r31.u64;
	// rlwinm r10,r11,1,0,30
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// srawi r31,r31,2
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x3) != 0);
	ctx.r31.s64 = ctx.r31.s32 >> 2;
	// add r11,r11,r10
	ctx.r11.u64 = ctx.r11.u64 + ctx.r10.u64;
	// rlwinm r10,r31,1,0,30
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 1) & 0xFFFFFFFE;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// subf r10,r10,r29
	ctx.r10.u64 = ctx.r29.u64 - ctx.r10.u64;
	// add r30,r11,r30
	ctx.r30.u64 = ctx.r11.u64 + ctx.r30.u64;
	// rlwinm r11,r10,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r3,r27
	ctx.r3.u64 = ctx.r27.u64;
	// add r5,r11,r28
	ctx.r5.u64 = ctx.r11.u64 + ctx.r28.u64;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// bl 0x8295bbb0
	ctx.lr = 0x8295DD30;
	sub_8295BBB0(ctx, base);
	// cmpwi cr6,r27,512
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 512, ctx.xer);
	// bgt cr6,0x8295dcb4
	if (ctx.cr6.gt) goto loc_8295DCB4;
loc_8295DD38:
	// mr r6,r28
	ctx.r6.u64 = ctx.r28.u64;
	// mr r5,r29
	ctx.r5.u64 = ctx.r29.u64;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r27
	ctx.r3.u64 = ctx.r27.u64;
	// bl 0x8295d4a0
	ctx.lr = 0x8295DD4C;
	sub_8295D4A0(ctx, base);
	// addi r1,r1,128
	ctx.r1.s64 = ctx.r1.s64 + 128;
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295DD58) {
	REX_FUNC_PROLOGUE(sub_8295DD58);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a4
	ctx.lr = 0x8295DD60;
	__savegprlr_27(ctx, base);
	// stwu r1,-128(r1)
	ea = -128 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r29,r3
	ctx.r29.u64 = ctx.r3.u64;
	// mr r28,r5
	ctx.r28.u64 = ctx.r5.u64;
	// mr r27,r6
	ctx.r27.u64 = ctx.r6.u64;
	// subf r11,r29,r28
	ctx.r11.u64 = ctx.r28.u64 - ctx.r29.u64;
	// mr r30,r4
	ctx.r30.u64 = ctx.r4.u64;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// srawi r31,r29,2
	ctx.xer.ca = (ctx.r29.s32 < 0) & ((ctx.r29.u32 & 0x3) != 0);
	ctx.r31.s64 = ctx.r29.s32 >> 2;
	// add r5,r11,r27
	ctx.r5.u64 = ctx.r11.u64 + ctx.r27.u64;
	// bl 0x8295bf38
	ctx.lr = 0x8295DD88;
	sub_8295BF38(ctx, base);
	// cmpwi cr6,r29,512
	ctx.cr6.compare<int32_t>(ctx.r29.s32, 512, ctx.xer);
	// ble cr6,0x8295de10
	if (!ctx.cr6.gt) goto loc_8295DE10;
loc_8295DD90:
	// mr r6,r27
	ctx.r6.u64 = ctx.r27.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295dc78
	ctx.lr = 0x8295DDA4;
	sub_8295DC78(ctx, base);
	// rlwinm r11,r31,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r6,r27
	ctx.r6.u64 = ctx.r27.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// add r4,r11,r30
	ctx.r4.u64 = ctx.r11.u64 + ctx.r30.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295dd58
	ctx.lr = 0x8295DDBC;
	sub_8295DD58(ctx, base);
	// rlwinm r11,r31,3,0,28
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 3) & 0xFFFFFFF8;
	// mr r6,r27
	ctx.r6.u64 = ctx.r27.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// add r4,r11,r30
	ctx.r4.u64 = ctx.r11.u64 + ctx.r30.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295dc78
	ctx.lr = 0x8295DDD4;
	sub_8295DC78(ctx, base);
	// mr r11,r31
	ctx.r11.u64 = ctx.r31.u64;
	// mr r29,r31
	ctx.r29.u64 = ctx.r31.u64;
	// rlwinm r10,r11,1,0,30
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 1) & 0xFFFFFFFE;
	// subf r9,r29,r28
	ctx.r9.u64 = ctx.r28.u64 - ctx.r29.u64;
	// add r11,r11,r10
	ctx.r11.u64 = ctx.r11.u64 + ctx.r10.u64;
	// rlwinm r10,r9,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r5,r10,r27
	ctx.r5.u64 = ctx.r10.u64 + ctx.r27.u64;
	// add r30,r11,r30
	ctx.r30.u64 = ctx.r11.u64 + ctx.r30.u64;
	// mr r3,r29
	ctx.r3.u64 = ctx.r29.u64;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// srawi r31,r31,2
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x3) != 0);
	ctx.r31.s64 = ctx.r31.s32 >> 2;
	// bl 0x8295bf38
	ctx.lr = 0x8295DE08;
	sub_8295BF38(ctx, base);
	// cmpwi cr6,r29,512
	ctx.cr6.compare<int32_t>(ctx.r29.s32, 512, ctx.xer);
	// bgt cr6,0x8295dd90
	if (ctx.cr6.gt) goto loc_8295DD90;
loc_8295DE10:
	// mr r6,r27
	ctx.r6.u64 = ctx.r27.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r29
	ctx.r3.u64 = ctx.r29.u64;
	// bl 0x8295d878
	ctx.lr = 0x8295DE24;
	sub_8295D878(ctx, base);
	// addi r1,r1,128
	ctx.r1.s64 = ctx.r1.s64 + 128;
	// b 0x824131f4
	__restgprlr_27(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295DE30) {
	REX_FUNC_PROLOGUE(sub_8295DE30);
	PPCRegister temp{};
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a0
	ctx.lr = 0x8295DE38;
	__savegprlr_26(ctx, base);
	// addi r12,r1,-56
	ctx.r12.s64 = ctx.r1.s64 + -56;
	// bl 0x824144b0
	ctx.lr = 0x8295DE40;
	__savefpr_22(ctx, base);
	// stwu r1,-224(r1)
	ea = -224 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r27,r3
	ctx.r27.u64 = ctx.r3.u64;
	// mr r31,r4
	ctx.r31.u64 = ctx.r4.u64;
	// mr r26,r5
	ctx.r26.u64 = ctx.r5.u64;
	// mr r28,r6
	ctx.r28.u64 = ctx.r6.u64;
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// cmpwi cr6,r27,32
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 32, ctx.xer);
	// ble cr6,0x8295df50
	if (!ctx.cr6.gt) goto loc_8295DF50;
	// srawi r30,r27,2
	ctx.xer.ca = (ctx.r27.s32 < 0) & ((ctx.r27.u32 & 0x3) != 0);
	ctx.r30.s64 = ctx.r27.s32 >> 2;
	// subf r11,r30,r28
	ctx.r11.u64 = ctx.r28.u64 - ctx.r30.u64;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r5,r11,r29
	ctx.r5.u64 = ctx.r11.u64 + ctx.r29.u64;
	// bl 0x8295ad70
	ctx.lr = 0x8295DE74;
	sub_8295AD70(ctx, base);
	// cmpwi cr6,r27,512
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 512, ctx.xer);
	// mr r6,r29
	ctx.r6.u64 = ctx.r29.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// ble cr6,0x8295defc
	if (!ctx.cr6.gt) goto loc_8295DEFC;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// bl 0x8295dc78
	ctx.lr = 0x8295DE8C;
	sub_8295DC78(ctx, base);
	// rlwinm r11,r30,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r6,r29
	ctx.r6.u64 = ctx.r29.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// add r4,r11,r31
	ctx.r4.u64 = ctx.r11.u64 + ctx.r31.u64;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// bl 0x8295dd58
	ctx.lr = 0x8295DEA4;
	sub_8295DD58(ctx, base);
	// rlwinm r11,r30,3,0,28
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 3) & 0xFFFFFFF8;
	// mr r6,r29
	ctx.r6.u64 = ctx.r29.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// add r4,r11,r31
	ctx.r4.u64 = ctx.r11.u64 + ctx.r31.u64;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// bl 0x8295dc78
	ctx.lr = 0x8295DEBC;
	sub_8295DC78(ctx, base);
	// rlwinm r11,r30,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 1) & 0xFFFFFFFE;
	// mr r6,r29
	ctx.r6.u64 = ctx.r29.u64;
	// add r11,r30,r11
	ctx.r11.u64 = ctx.r30.u64 + ctx.r11.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// add r4,r11,r31
	ctx.r4.u64 = ctx.r11.u64 + ctx.r31.u64;
	// bl 0x8295dc78
	ctx.lr = 0x8295DEDC;
	sub_8295DC78(ctx, base);
	// mr r5,r31
	ctx.r5.u64 = ctx.r31.u64;
	// mr r4,r26
	ctx.r4.u64 = ctx.r26.u64;
	// mr r3,r27
	ctx.r3.u64 = ctx.r27.u64;
	// bl 0x8295a108
	ctx.lr = 0x8295DEEC;
	sub_8295A108(ctx, base);
	// addi r1,r1,224
	ctx.r1.s64 = ctx.r1.s64 + 224;
	// addi r12,r1,-56
	ctx.r12.s64 = ctx.r1.s64 + -56;
	// bl 0x824144fc
	ctx.lr = 0x8295DEF8;
	__restfpr_22(ctx, base);
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295DEFC:
	// cmpwi cr6,r30,32
	ctx.cr6.compare<int32_t>(ctx.r30.s32, 32, ctx.xer);
	// mr r3,r27
	ctx.r3.u64 = ctx.r27.u64;
	// ble cr6,0x8295df2c
	if (!ctx.cr6.gt) goto loc_8295DF2C;
	// bl 0x8295d4a0
	ctx.lr = 0x8295DF0C;
	sub_8295D4A0(ctx, base);
	// mr r5,r31
	ctx.r5.u64 = ctx.r31.u64;
	// mr r4,r26
	ctx.r4.u64 = ctx.r26.u64;
	// mr r3,r27
	ctx.r3.u64 = ctx.r27.u64;
	// bl 0x8295a108
	ctx.lr = 0x8295DF1C;
	sub_8295A108(ctx, base);
	// addi r1,r1,224
	ctx.r1.s64 = ctx.r1.s64 + 224;
	// addi r12,r1,-56
	ctx.r12.s64 = ctx.r1.s64 + -56;
	// bl 0x824144fc
	ctx.lr = 0x8295DF28;
	__restfpr_22(ctx, base);
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295DF2C:
	// bl 0x8295d400
	ctx.lr = 0x8295DF30;
	sub_8295D400(ctx, base);
	// mr r5,r31
	ctx.r5.u64 = ctx.r31.u64;
	// mr r4,r26
	ctx.r4.u64 = ctx.r26.u64;
	// mr r3,r27
	ctx.r3.u64 = ctx.r27.u64;
	// bl 0x8295a108
	ctx.lr = 0x8295DF40;
	sub_8295A108(ctx, base);
	// addi r1,r1,224
	ctx.r1.s64 = ctx.r1.s64 + 224;
	// addi r12,r1,-56
	ctx.r12.s64 = ctx.r1.s64 + -56;
	// bl 0x824144fc
	ctx.lr = 0x8295DF4C;
	__restfpr_22(ctx, base);
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295DF50:
	// cmpwi cr6,r27,8
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 8, ctx.xer);
	// ble cr6,0x8295e09c
	if (!ctx.cr6.gt) goto loc_8295E09C;
	// cmpwi cr6,r27,32
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 32, ctx.xer);
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bne cr6,0x8295e044
	if (!ctx.cr6.eq) goto loc_8295E044;
	// addi r11,r28,-8
	ctx.r11.s64 = ctx.r28.s64 + -8;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r29
	ctx.r4.u64 = ctx.r11.u64 + ctx.r29.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295DF74;
	sub_8295C378(ctx, base);
	// lfs f0,8(r31)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 8);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,12(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 12);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,16(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 16);
	ctx.f12.f64 = double(temp.f32);
	// lfs f11,20(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 20);
	ctx.f11.f64 = double(temp.f32);
	// lfs f10,24(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 24);
	ctx.f10.f64 = double(temp.f32);
	// lfs f9,40(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 40);
	ctx.f9.f64 = double(temp.f32);
	// lfs f8,44(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 44);
	ctx.f8.f64 = double(temp.f32);
	// lfs f7,28(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 28);
	ctx.f7.f64 = double(temp.f32);
	// lfs f6,56(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 56);
	ctx.f6.f64 = double(temp.f32);
	// lfs f5,60(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 60);
	ctx.f5.f64 = double(temp.f32);
	// lfs f4,88(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 88);
	ctx.f4.f64 = double(temp.f32);
	// lfs f3,92(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 92);
	ctx.f3.f64 = double(temp.f32);
	// lfs f2,64(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 64);
	ctx.f2.f64 = double(temp.f32);
	// lfs f1,68(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 68);
	ctx.f1.f64 = double(temp.f32);
	// lfs f31,32(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 32);
	ctx.f31.f64 = double(temp.f32);
	// lfs f30,36(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 36);
	ctx.f30.f64 = double(temp.f32);
	// lfs f29,96(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 96);
	ctx.f29.f64 = double(temp.f32);
	// lfs f28,80(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 80);
	ctx.f28.f64 = double(temp.f32);
	// lfs f27,84(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 84);
	ctx.f27.f64 = double(temp.f32);
	// lfs f26,100(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 100);
	ctx.f26.f64 = double(temp.f32);
	// lfs f25,112(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 112);
	ctx.f25.f64 = double(temp.f32);
	// lfs f24,116(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 116);
	ctx.f24.f64 = double(temp.f32);
	// lfs f23,104(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 104);
	ctx.f23.f64 = double(temp.f32);
	// lfs f22,108(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 108);
	ctx.f22.f64 = double(temp.f32);
	// stfs f2,8(r31)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r31.u32 + 8, temp.u32);
	// stfs f1,12(r31)
	temp.f32 = float(ctx.f1.f64);
	REX_STORE_U32(ctx.r31.u32 + 12, temp.u32);
	// stfs f31,16(r31)
	temp.f32 = float(ctx.f31.f64);
	REX_STORE_U32(ctx.r31.u32 + 16, temp.u32);
	// stfs f30,20(r31)
	temp.f32 = float(ctx.f30.f64);
	REX_STORE_U32(ctx.r31.u32 + 20, temp.u32);
	// stfs f29,24(r31)
	temp.f32 = float(ctx.f29.f64);
	REX_STORE_U32(ctx.r31.u32 + 24, temp.u32);
	// stfs f26,28(r31)
	temp.f32 = float(ctx.f26.f64);
	REX_STORE_U32(ctx.r31.u32 + 28, temp.u32);
	// stfs f28,40(r31)
	temp.f32 = float(ctx.f28.f64);
	REX_STORE_U32(ctx.r31.u32 + 40, temp.u32);
	// stfs f27,44(r31)
	temp.f32 = float(ctx.f27.f64);
	REX_STORE_U32(ctx.r31.u32 + 44, temp.u32);
	// stfs f25,56(r31)
	temp.f32 = float(ctx.f25.f64);
	REX_STORE_U32(ctx.r31.u32 + 56, temp.u32);
	// stfs f24,60(r31)
	temp.f32 = float(ctx.f24.f64);
	REX_STORE_U32(ctx.r31.u32 + 60, temp.u32);
	// stfs f23,88(r31)
	temp.f32 = float(ctx.f23.f64);
	REX_STORE_U32(ctx.r31.u32 + 88, temp.u32);
	// stfs f22,92(r31)
	temp.f32 = float(ctx.f22.f64);
	REX_STORE_U32(ctx.r31.u32 + 92, temp.u32);
	// stfs f0,64(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 64, temp.u32);
	// stfs f13,68(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 68, temp.u32);
	// stfs f12,32(r31)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r31.u32 + 32, temp.u32);
	// stfs f11,36(r31)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r31.u32 + 36, temp.u32);
	// stfs f10,96(r31)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r31.u32 + 96, temp.u32);
	// stfs f9,80(r31)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r31.u32 + 80, temp.u32);
	// stfs f8,84(r31)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r31.u32 + 84, temp.u32);
	// stfs f7,100(r31)
	temp.f32 = float(ctx.f7.f64);
	REX_STORE_U32(ctx.r31.u32 + 100, temp.u32);
	// stfs f4,104(r31)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r31.u32 + 104, temp.u32);
	// stfs f3,108(r31)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r31.u32 + 108, temp.u32);
	// stfs f6,112(r31)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r31.u32 + 112, temp.u32);
	// stfs f5,116(r31)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r31.u32 + 116, temp.u32);
	// addi r1,r1,224
	ctx.r1.s64 = ctx.r1.s64 + 224;
	// addi r12,r1,-56
	ctx.r12.s64 = ctx.r1.s64 + -56;
	// bl 0x824144fc
	ctx.lr = 0x8295E040;
	__restfpr_22(ctx, base);
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295E044:
	// mr r4,r29
	ctx.r4.u64 = ctx.r29.u64;
	// bl 0x8295ccd8
	ctx.lr = 0x8295E04C;
	sub_8295CCD8(ctx, base);
	// lfs f0,8(r31)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 8);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,12(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 12);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,24(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 24);
	ctx.f12.f64 = double(temp.f32);
	// lfs f11,28(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 28);
	ctx.f11.f64 = double(temp.f32);
	// lfs f10,32(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 32);
	ctx.f10.f64 = double(temp.f32);
	// lfs f9,36(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 36);
	ctx.f9.f64 = double(temp.f32);
	// lfs f8,48(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 48);
	ctx.f8.f64 = double(temp.f32);
	// lfs f7,52(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 52);
	ctx.f7.f64 = double(temp.f32);
	// stfs f9,12(r31)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r31.u32 + 12, temp.u32);
	// stfs f8,24(r31)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r31.u32 + 24, temp.u32);
	// stfs f7,28(r31)
	temp.f32 = float(ctx.f7.f64);
	REX_STORE_U32(ctx.r31.u32 + 28, temp.u32);
	// stfs f0,32(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 32, temp.u32);
	// stfs f13,36(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 36, temp.u32);
	// stfs f12,48(r31)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r31.u32 + 48, temp.u32);
	// stfs f11,52(r31)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r31.u32 + 52, temp.u32);
	// stfs f10,8(r31)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r31.u32 + 8, temp.u32);
	// addi r1,r1,224
	ctx.r1.s64 = ctx.r1.s64 + 224;
	// addi r12,r1,-56
	ctx.r12.s64 = ctx.r1.s64 + -56;
	// bl 0x824144fc
	ctx.lr = 0x8295E098;
	__restfpr_22(ctx, base);
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295E09C:
	// bne cr6,0x8295e130
	if (!ctx.cr6.eq) goto loc_8295E130;
	// lfs f13,16(r31)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 16);
	ctx.f13.f64 = double(temp.f32);
	// lfs f0,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// fadds f6,f0,f13
	ctx.f6.f64 = double(float(ctx.f0.f64 + ctx.f13.f64));
	// lfs f11,4(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 4);
	ctx.f11.f64 = double(temp.f32);
	// lfs f12,20(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 20);
	ctx.f12.f64 = double(temp.f32);
	// fsubs f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// lfs f9,8(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 8);
	ctx.f9.f64 = double(temp.f32);
	// fadds f13,f11,f12
	ctx.f13.f64 = double(float(ctx.f11.f64 + ctx.f12.f64));
	// lfs f10,24(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 24);
	ctx.f10.f64 = double(temp.f32);
	// fsubs f12,f11,f12
	ctx.f12.f64 = double(float(ctx.f11.f64 - ctx.f12.f64));
	// fadds f11,f10,f9
	ctx.f11.f64 = double(float(ctx.f10.f64 + ctx.f9.f64));
	// lfs f7,12(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 12);
	ctx.f7.f64 = double(temp.f32);
	// lfs f8,28(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 28);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f10,f9,f10
	ctx.f10.f64 = double(float(ctx.f9.f64 - ctx.f10.f64));
	// fadds f9,f8,f7
	ctx.f9.f64 = double(float(ctx.f8.f64 + ctx.f7.f64));
	// fsubs f8,f7,f8
	ctx.f8.f64 = double(float(ctx.f7.f64 - ctx.f8.f64));
	// fadds f7,f11,f6
	ctx.f7.f64 = double(float(ctx.f11.f64 + ctx.f6.f64));
	// stfs f7,0(r31)
	temp.f32 = float(ctx.f7.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// fsubs f11,f6,f11
	ctx.f11.f64 = double(float(ctx.f6.f64 - ctx.f11.f64));
	// stfs f11,16(r31)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r31.u32 + 16, temp.u32);
	// fadds f11,f9,f13
	ctx.f11.f64 = double(float(ctx.f9.f64 + ctx.f13.f64));
	// stfs f11,4(r31)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r31.u32 + 4, temp.u32);
	// fsubs f13,f13,f9
	ctx.f13.f64 = double(float(ctx.f13.f64 - ctx.f9.f64));
	// stfs f13,20(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 20, temp.u32);
	// fsubs f13,f0,f8
	ctx.f13.f64 = double(float(ctx.f0.f64 - ctx.f8.f64));
	// stfs f13,8(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 8, temp.u32);
	// fadds f0,f8,f0
	ctx.f0.f64 = double(float(ctx.f8.f64 + ctx.f0.f64));
	// stfs f0,24(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 24, temp.u32);
	// fadds f13,f10,f12
	ctx.f13.f64 = double(float(ctx.f10.f64 + ctx.f12.f64));
	// stfs f13,12(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 12, temp.u32);
	// fsubs f0,f12,f10
	ctx.f0.f64 = double(float(ctx.f12.f64 - ctx.f10.f64));
	// stfs f0,28(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 28, temp.u32);
	// addi r1,r1,224
	ctx.r1.s64 = ctx.r1.s64 + 224;
	// addi r12,r1,-56
	ctx.r12.s64 = ctx.r1.s64 + -56;
	// bl 0x824144fc
	ctx.lr = 0x8295E12C;
	__restfpr_22(ctx, base);
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295E130:
	// cmpwi cr6,r27,4
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 4, ctx.xer);
	// bne cr6,0x8295e168
	if (!ctx.cr6.eq) goto loc_8295E168;
	// lfs f13,8(r31)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 8);
	ctx.f13.f64 = double(temp.f32);
	// lfs f0,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// fsubs f10,f0,f13
	ctx.f10.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// lfs f12,4(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// lfs f11,12(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 12);
	ctx.f11.f64 = double(temp.f32);
	// fadds f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f13.f64));
	// stfs f0,0(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// fadds f13,f12,f11
	ctx.f13.f64 = double(float(ctx.f12.f64 + ctx.f11.f64));
	// fsubs f0,f12,f11
	ctx.f0.f64 = double(float(ctx.f12.f64 - ctx.f11.f64));
	// stfs f13,4(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 4, temp.u32);
	// stfs f0,12(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 12, temp.u32);
	// stfs f10,8(r31)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r31.u32 + 8, temp.u32);
loc_8295E168:
	// addi r1,r1,224
	ctx.r1.s64 = ctx.r1.s64 + 224;
	// addi r12,r1,-56
	ctx.r12.s64 = ctx.r1.s64 + -56;
	// bl 0x824144fc
	ctx.lr = 0x8295E174;
	__restfpr_22(ctx, base);
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295E178) {
	REX_FUNC_PROLOGUE(sub_8295E178);
	PPCRegister temp{};
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x824131a0
	ctx.lr = 0x8295E180;
	__savegprlr_26(ctx, base);
	// stwu r1,-144(r1)
	ea = -144 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r27,r3
	ctx.r27.u64 = ctx.r3.u64;
	// mr r31,r4
	ctx.r31.u64 = ctx.r4.u64;
	// mr r26,r5
	ctx.r26.u64 = ctx.r5.u64;
	// mr r28,r6
	ctx.r28.u64 = ctx.r6.u64;
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// cmpwi cr6,r27,32
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 32, ctx.xer);
	// ble cr6,0x8295e278
	if (!ctx.cr6.gt) goto loc_8295E278;
	// srawi r30,r27,2
	ctx.xer.ca = (ctx.r27.s32 < 0) & ((ctx.r27.u32 & 0x3) != 0);
	ctx.r30.s64 = ctx.r27.s32 >> 2;
	// subf r11,r30,r28
	ctx.r11.u64 = ctx.r28.u64 - ctx.r30.u64;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r5,r11,r29
	ctx.r5.u64 = ctx.r11.u64 + ctx.r29.u64;
	// bl 0x8295b488
	ctx.lr = 0x8295E1B4;
	sub_8295B488(ctx, base);
	// cmpwi cr6,r27,512
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 512, ctx.xer);
	// mr r6,r29
	ctx.r6.u64 = ctx.r29.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// ble cr6,0x8295e234
	if (!ctx.cr6.gt) goto loc_8295E234;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// bl 0x8295dc78
	ctx.lr = 0x8295E1CC;
	sub_8295DC78(ctx, base);
	// rlwinm r11,r30,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r6,r29
	ctx.r6.u64 = ctx.r29.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// add r4,r11,r31
	ctx.r4.u64 = ctx.r11.u64 + ctx.r31.u64;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// bl 0x8295dd58
	ctx.lr = 0x8295E1E4;
	sub_8295DD58(ctx, base);
	// rlwinm r11,r30,3,0,28
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 3) & 0xFFFFFFF8;
	// mr r6,r29
	ctx.r6.u64 = ctx.r29.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// add r4,r11,r31
	ctx.r4.u64 = ctx.r11.u64 + ctx.r31.u64;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// bl 0x8295dc78
	ctx.lr = 0x8295E1FC;
	sub_8295DC78(ctx, base);
	// rlwinm r11,r30,1,0,30
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 1) & 0xFFFFFFFE;
	// mr r6,r29
	ctx.r6.u64 = ctx.r29.u64;
	// add r11,r30,r11
	ctx.r11.u64 = ctx.r30.u64 + ctx.r11.u64;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// add r4,r11,r31
	ctx.r4.u64 = ctx.r11.u64 + ctx.r31.u64;
	// bl 0x8295dc78
	ctx.lr = 0x8295E21C;
	sub_8295DC78(ctx, base);
	// mr r5,r31
	ctx.r5.u64 = ctx.r31.u64;
	// mr r4,r26
	ctx.r4.u64 = ctx.r26.u64;
	// mr r3,r27
	ctx.r3.u64 = ctx.r27.u64;
	// bl 0x8295a638
	ctx.lr = 0x8295E22C;
	sub_8295A638(ctx, base);
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295E234:
	// cmpwi cr6,r30,32
	ctx.cr6.compare<int32_t>(ctx.r30.s32, 32, ctx.xer);
	// mr r3,r27
	ctx.r3.u64 = ctx.r27.u64;
	// ble cr6,0x8295e25c
	if (!ctx.cr6.gt) goto loc_8295E25C;
	// bl 0x8295d4a0
	ctx.lr = 0x8295E244;
	sub_8295D4A0(ctx, base);
	// mr r5,r31
	ctx.r5.u64 = ctx.r31.u64;
	// mr r4,r26
	ctx.r4.u64 = ctx.r26.u64;
	// mr r3,r27
	ctx.r3.u64 = ctx.r27.u64;
	// bl 0x8295a638
	ctx.lr = 0x8295E254;
	sub_8295A638(ctx, base);
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295E25C:
	// bl 0x8295d400
	ctx.lr = 0x8295E260;
	sub_8295D400(ctx, base);
	// mr r5,r31
	ctx.r5.u64 = ctx.r31.u64;
	// mr r4,r26
	ctx.r4.u64 = ctx.r26.u64;
	// mr r3,r27
	ctx.r3.u64 = ctx.r27.u64;
	// bl 0x8295a638
	ctx.lr = 0x8295E270;
	sub_8295A638(ctx, base);
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295E278:
	// cmpwi cr6,r27,8
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 8, ctx.xer);
	// ble cr6,0x8295e328
	if (!ctx.cr6.gt) goto loc_8295E328;
	// cmpwi cr6,r27,32
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 32, ctx.xer);
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bne cr6,0x8295e2a8
	if (!ctx.cr6.eq) goto loc_8295E2A8;
	// addi r11,r28,-8
	ctx.r11.s64 = ctx.r28.s64 + -8;
	// rlwinm r11,r11,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// add r4,r11,r29
	ctx.r4.u64 = ctx.r11.u64 + ctx.r29.u64;
	// bl 0x8295c378
	ctx.lr = 0x8295E29C;
	sub_8295C378(ctx, base);
	// bl 0x8295ac58
	ctx.lr = 0x8295E2A0;
	sub_8295AC58(ctx, base);
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295E2A8:
	// mr r4,r29
	ctx.r4.u64 = ctx.r29.u64;
	// bl 0x8295ccd8
	ctx.lr = 0x8295E2B0;
	sub_8295CCD8(ctx, base);
	// lfs f0,8(r31)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 8);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,12(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 12);
	ctx.f13.f64 = double(temp.f32);
	// lfs f12,16(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 16);
	ctx.f12.f64 = double(temp.f32);
	// lfs f11,20(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 20);
	ctx.f11.f64 = double(temp.f32);
	// lfs f10,32(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 32);
	ctx.f10.f64 = double(temp.f32);
	// lfs f9,36(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 36);
	ctx.f9.f64 = double(temp.f32);
	// lfs f8,56(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 56);
	ctx.f8.f64 = double(temp.f32);
	// lfs f7,60(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 60);
	ctx.f7.f64 = double(temp.f32);
	// lfs f6,24(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 24);
	ctx.f6.f64 = double(temp.f32);
	// lfs f5,28(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 28);
	ctx.f5.f64 = double(temp.f32);
	// lfs f4,40(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 40);
	ctx.f4.f64 = double(temp.f32);
	// lfs f3,44(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 44);
	ctx.f3.f64 = double(temp.f32);
	// lfs f2,48(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 48);
	ctx.f2.f64 = double(temp.f32);
	// lfs f1,52(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 52);
	ctx.f1.f64 = double(temp.f32);
	// stfs f8,8(r31)
	temp.f32 = float(ctx.f8.f64);
	REX_STORE_U32(ctx.r31.u32 + 8, temp.u32);
	// stfs f7,12(r31)
	temp.f32 = float(ctx.f7.f64);
	REX_STORE_U32(ctx.r31.u32 + 12, temp.u32);
	// stfs f6,16(r31)
	temp.f32 = float(ctx.f6.f64);
	REX_STORE_U32(ctx.r31.u32 + 16, temp.u32);
	// stfs f5,20(r31)
	temp.f32 = float(ctx.f5.f64);
	REX_STORE_U32(ctx.r31.u32 + 20, temp.u32);
	// stfs f4,24(r31)
	temp.f32 = float(ctx.f4.f64);
	REX_STORE_U32(ctx.r31.u32 + 24, temp.u32);
	// stfs f3,28(r31)
	temp.f32 = float(ctx.f3.f64);
	REX_STORE_U32(ctx.r31.u32 + 28, temp.u32);
	// stfs f2,40(r31)
	temp.f32 = float(ctx.f2.f64);
	REX_STORE_U32(ctx.r31.u32 + 40, temp.u32);
	// stfs f1,44(r31)
	temp.f32 = float(ctx.f1.f64);
	REX_STORE_U32(ctx.r31.u32 + 44, temp.u32);
	// stfs f0,32(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 32, temp.u32);
	// stfs f13,36(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 36, temp.u32);
	// stfs f12,48(r31)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r31.u32 + 48, temp.u32);
	// stfs f11,52(r31)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r31.u32 + 52, temp.u32);
	// stfs f10,56(r31)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r31.u32 + 56, temp.u32);
	// stfs f9,60(r31)
	temp.f32 = float(ctx.f9.f64);
	REX_STORE_U32(ctx.r31.u32 + 60, temp.u32);
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295E328:
	// bne cr6,0x8295e3b4
	if (!ctx.cr6.eq) goto loc_8295E3B4;
	// lfs f13,16(r31)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 16);
	ctx.f13.f64 = double(temp.f32);
	// lfs f0,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// fadds f6,f0,f13
	ctx.f6.f64 = double(float(ctx.f0.f64 + ctx.f13.f64));
	// lfs f11,4(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 4);
	ctx.f11.f64 = double(temp.f32);
	// lfs f12,20(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 20);
	ctx.f12.f64 = double(temp.f32);
	// fsubs f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// lfs f9,8(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 8);
	ctx.f9.f64 = double(temp.f32);
	// fadds f13,f11,f12
	ctx.f13.f64 = double(float(ctx.f11.f64 + ctx.f12.f64));
	// lfs f10,24(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 24);
	ctx.f10.f64 = double(temp.f32);
	// fsubs f12,f11,f12
	ctx.f12.f64 = double(float(ctx.f11.f64 - ctx.f12.f64));
	// fadds f11,f10,f9
	ctx.f11.f64 = double(float(ctx.f10.f64 + ctx.f9.f64));
	// lfs f7,12(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 12);
	ctx.f7.f64 = double(temp.f32);
	// lfs f8,28(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 28);
	ctx.f8.f64 = double(temp.f32);
	// fsubs f10,f9,f10
	ctx.f10.f64 = double(float(ctx.f9.f64 - ctx.f10.f64));
	// fadds f9,f8,f7
	ctx.f9.f64 = double(float(ctx.f8.f64 + ctx.f7.f64));
	// fsubs f8,f7,f8
	ctx.f8.f64 = double(float(ctx.f7.f64 - ctx.f8.f64));
	// fadds f7,f11,f6
	ctx.f7.f64 = double(float(ctx.f11.f64 + ctx.f6.f64));
	// stfs f7,0(r31)
	temp.f32 = float(ctx.f7.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// fsubs f11,f6,f11
	ctx.f11.f64 = double(float(ctx.f6.f64 - ctx.f11.f64));
	// stfs f11,16(r31)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r31.u32 + 16, temp.u32);
	// fadds f11,f9,f13
	ctx.f11.f64 = double(float(ctx.f9.f64 + ctx.f13.f64));
	// stfs f11,4(r31)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r31.u32 + 4, temp.u32);
	// fsubs f13,f13,f9
	ctx.f13.f64 = double(float(ctx.f13.f64 - ctx.f9.f64));
	// stfs f13,20(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 20, temp.u32);
	// fadds f13,f8,f0
	ctx.f13.f64 = double(float(ctx.f8.f64 + ctx.f0.f64));
	// stfs f13,8(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 8, temp.u32);
	// fsubs f0,f0,f8
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f8.f64));
	// stfs f0,24(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 24, temp.u32);
	// fsubs f13,f12,f10
	ctx.f13.f64 = double(float(ctx.f12.f64 - ctx.f10.f64));
	// stfs f13,12(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 12, temp.u32);
	// fadds f0,f10,f12
	ctx.f0.f64 = double(float(ctx.f10.f64 + ctx.f12.f64));
	// stfs f0,28(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 28, temp.u32);
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
loc_8295E3B4:
	// cmpwi cr6,r27,4
	ctx.cr6.compare<int32_t>(ctx.r27.s32, 4, ctx.xer);
	// bne cr6,0x8295e3ec
	if (!ctx.cr6.eq) goto loc_8295E3EC;
	// lfs f13,8(r31)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 8);
	ctx.f13.f64 = double(temp.f32);
	// lfs f0,0(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// fsubs f10,f0,f13
	ctx.f10.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// lfs f12,4(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 4);
	ctx.f12.f64 = double(temp.f32);
	// lfs f11,12(r31)
	temp.u32 = REX_LOAD_U32(ctx.r31.u32 + 12);
	ctx.f11.f64 = double(temp.f32);
	// fadds f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f13.f64));
	// stfs f0,0(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 0, temp.u32);
	// fadds f13,f12,f11
	ctx.f13.f64 = double(float(ctx.f12.f64 + ctx.f11.f64));
	// fsubs f0,f12,f11
	ctx.f0.f64 = double(float(ctx.f12.f64 - ctx.f11.f64));
	// stfs f13,4(r31)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r31.u32 + 4, temp.u32);
	// stfs f10,8(r31)
	temp.f32 = float(ctx.f10.f64);
	REX_STORE_U32(ctx.r31.u32 + 8, temp.u32);
	// stfs f0,12(r31)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r31.u32 + 12, temp.u32);
loc_8295E3EC:
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131f0
	__restgprlr_26(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295E3F8) {
	REX_FUNC_PROLOGUE(sub_8295E3F8);
	PPCRegister temp{};
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x8241319c
	ctx.lr = 0x8295E400;
	__savegprlr_25(ctx, base);
	// stwu r1,-144(r1)
	ea = -144 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r27,r6
	ctx.r27.u64 = ctx.r6.u64;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// mr r25,r4
	ctx.r25.u64 = ctx.r4.u64;
	// mr r28,r5
	ctx.r28.u64 = ctx.r5.u64;
	// mr r29,r7
	ctx.r29.u64 = ctx.r7.u64;
	// lwz r30,0(r27)
	ctx.r30.u64 = REX_LOAD_U32(ctx.r27.u32 + 0);
	// rlwinm r11,r30,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// cmpw cr6,r31,r11
	ctx.cr6.compare<int32_t>(ctx.r31.s32, ctx.r11.s32, ctx.xer);
	// ble cr6,0x8295e43c
	if (!ctx.cr6.gt) goto loc_8295E43C;
	// srawi r30,r31,2
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x3) != 0);
	ctx.r30.s64 = ctx.r31.s32 >> 2;
	// mr r5,r29
	ctx.r5.u64 = ctx.r29.u64;
	// mr r4,r27
	ctx.r4.u64 = ctx.r27.u64;
	// mr r3,r30
	ctx.r3.u64 = ctx.r30.u64;
	// bl 0x82959de8
	ctx.lr = 0x8295E43C;
	sub_82959DE8(ctx, base);
loc_8295E43C:
	// lwz r26,4(r27)
	ctx.r26.u64 = REX_LOAD_U32(ctx.r27.u32 + 4);
	// rlwinm r11,r26,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r26.u32 | (ctx.r26.u64 << 32), 2) & 0xFFFFFFFC;
	// cmpw cr6,r31,r11
	ctx.cr6.compare<int32_t>(ctx.r31.s32, ctx.r11.s32, ctx.xer);
	// ble cr6,0x8295e464
	if (!ctx.cr6.gt) goto loc_8295E464;
	// rlwinm r11,r30,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// srawi r26,r31,2
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x3) != 0);
	ctx.r26.s64 = ctx.r31.s32 >> 2;
	// add r5,r11,r29
	ctx.r5.u64 = ctx.r11.u64 + ctx.r29.u64;
	// mr r4,r27
	ctx.r4.u64 = ctx.r27.u64;
	// mr r3,r26
	ctx.r3.u64 = ctx.r26.u64;
	// bl 0x8295a008
	ctx.lr = 0x8295E464;
	sub_8295A008(ctx, base);
loc_8295E464:
	// cmpwi cr6,r25,0
	ctx.cr6.compare<int32_t>(ctx.r25.s32, 0, ctx.xer);
	// blt cr6,0x8295e4e4
	if (ctx.cr6.lt) goto loc_8295E4E4;
	// cmpwi cr6,r31,4
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 4, ctx.xer);
	// ble cr6,0x8295e4a8
	if (!ctx.cr6.gt) goto loc_8295E4A8;
	// mr r7,r29
	ctx.r7.u64 = ctx.r29.u64;
	// mr r6,r30
	ctx.r6.u64 = ctx.r30.u64;
	// addi r5,r27,8
	ctx.r5.s64 = ctx.r27.s64 + 8;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295de30
	ctx.lr = 0x8295E48C;
	sub_8295DE30(ctx, base);
	// rlwinm r11,r30,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r5,r26
	ctx.r5.u64 = ctx.r26.u64;
	// add r6,r11,r29
	ctx.r6.u64 = ctx.r11.u64 + ctx.r29.u64;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295d050
	ctx.lr = 0x8295E4A4;
	sub_8295D050(ctx, base);
	// b 0x8295e4c4
	goto loc_8295E4C4;
loc_8295E4A8:
	// bne cr6,0x8295e4c4
	if (!ctx.cr6.eq) goto loc_8295E4C4;
	// mr r7,r29
	ctx.r7.u64 = ctx.r29.u64;
	// mr r6,r30
	ctx.r6.u64 = ctx.r30.u64;
	// addi r5,r27,8
	ctx.r5.s64 = ctx.r27.s64 + 8;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// li r3,4
	ctx.r3.s64 = 4;
	// bl 0x8295de30
	ctx.lr = 0x8295E4C4;
	sub_8295DE30(ctx, base);
loc_8295E4C4:
	// lfs f0,0(r28)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r28.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lfs f13,4(r28)
	temp.u32 = REX_LOAD_U32(ctx.r28.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// fsubs f12,f0,f13
	ctx.f12.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// stfs f12,4(r28)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r28.u32 + 4, temp.u32);
	// fadds f0,f13,f0
	ctx.f0.f64 = double(float(ctx.f13.f64 + ctx.f0.f64));
	// stfs f0,0(r28)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r28.u32 + 0, temp.u32);
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131ec
	__restgprlr_25(ctx, base);
	return;
loc_8295E4E4:
	// lfs f0,0(r28)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r28.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// lis r11,-32256
	ctx.r11.s64 = -2113929216;
	// lfs f13,4(r28)
	temp.u32 = REX_LOAD_U32(ctx.r28.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// cmpwi cr6,r31,4
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 4, ctx.xer);
	// fsubs f12,f0,f13
	ctx.f12.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// lfs f13,3636(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 3636);
	ctx.f13.f64 = double(temp.f32);
	// fmuls f13,f12,f13
	ctx.f13.f64 = double(float(ctx.f12.f64 * ctx.f13.f64));
	// stfs f13,4(r28)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r28.u32 + 4, temp.u32);
	// fsubs f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// stfs f0,0(r28)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r28.u32 + 0, temp.u32);
	// ble cr6,0x8295e530
	if (!ctx.cr6.gt) goto loc_8295E530;
	// rlwinm r11,r30,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r30.u32 | (ctx.r30.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r5,r26
	ctx.r5.u64 = ctx.r26.u64;
	// add r6,r11,r29
	ctx.r6.u64 = ctx.r11.u64 + ctx.r29.u64;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295d120
	ctx.lr = 0x8295E528;
	sub_8295D120(ctx, base);
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// b 0x8295e538
	goto loc_8295E538;
loc_8295E530:
	// bne cr6,0x8295e54c
	if (!ctx.cr6.eq) goto loc_8295E54C;
	// li r3,4
	ctx.r3.s64 = 4;
loc_8295E538:
	// mr r7,r29
	ctx.r7.u64 = ctx.r29.u64;
	// mr r6,r30
	ctx.r6.u64 = ctx.r30.u64;
	// addi r5,r27,8
	ctx.r5.s64 = ctx.r27.s64 + 8;
	// mr r4,r28
	ctx.r4.u64 = ctx.r28.u64;
	// bl 0x8295e178
	ctx.lr = 0x8295E54C;
	sub_8295E178(ctx, base);
loc_8295E54C:
	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x824131ec
	__restgprlr_25(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295E558) {
	REX_FUNC_PROLOGUE(sub_8295E558);
	PPCRegister temp{};
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413198
	ctx.lr = 0x8295E560;
	__savegprlr_24(ctx, base);
	// stwu r1,-160(r1)
	ea = -160 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r26,r6
	ctx.r26.u64 = ctx.r6.u64;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// mr r24,r4
	ctx.r24.u64 = ctx.r4.u64;
	// mr r30,r5
	ctx.r30.u64 = ctx.r5.u64;
	// mr r28,r7
	ctx.r28.u64 = ctx.r7.u64;
	// lwz r29,0(r26)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r26.u32 + 0);
	// rlwinm r11,r29,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// cmpw cr6,r31,r11
	ctx.cr6.compare<int32_t>(ctx.r31.s32, ctx.r11.s32, ctx.xer);
	// ble cr6,0x8295e59c
	if (!ctx.cr6.gt) goto loc_8295E59C;
	// srawi r29,r31,2
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x3) != 0);
	ctx.r29.s64 = ctx.r31.s32 >> 2;
	// mr r5,r28
	ctx.r5.u64 = ctx.r28.u64;
	// mr r4,r26
	ctx.r4.u64 = ctx.r26.u64;
	// mr r3,r29
	ctx.r3.u64 = ctx.r29.u64;
	// bl 0x82959de8
	ctx.lr = 0x8295E59C;
	sub_82959DE8(ctx, base);
loc_8295E59C:
	// lwz r25,4(r26)
	ctx.r25.u64 = REX_LOAD_U32(ctx.r26.u32 + 4);
	// cmpw cr6,r31,r25
	ctx.cr6.compare<int32_t>(ctx.r31.s32, ctx.r25.s32, ctx.xer);
	// ble cr6,0x8295e5c0
	if (!ctx.cr6.gt) goto loc_8295E5C0;
	// rlwinm r11,r29,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r4,r26
	ctx.r4.u64 = ctx.r26.u64;
	// add r5,r11,r28
	ctx.r5.u64 = ctx.r11.u64 + ctx.r28.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mr r25,r31
	ctx.r25.u64 = ctx.r31.u64;
	// bl 0x8295a008
	ctx.lr = 0x8295E5C0;
	sub_8295A008(ctx, base);
loc_8295E5C0:
	// cmpwi cr6,r24,0
	ctx.cr6.compare<int32_t>(ctx.r24.s32, 0, ctx.xer);
	// bge cr6,0x8295e670
	if (!ctx.cr6.lt) goto loc_8295E670;
	// rlwinm r10,r31,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r11,r31,-2
	ctx.r11.s64 = ctx.r31.s64 + -2;
	// add r10,r10,r30
	ctx.r10.u64 = ctx.r10.u64 + ctx.r30.u64;
	// cmpwi cr6,r11,2
	ctx.cr6.compare<int32_t>(ctx.r11.s32, 2, ctx.xer);
	// lfs f12,-4(r10)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r10.u32 + -4);
	ctx.f12.f64 = double(temp.f32);
	// blt cr6,0x8295e618
	if (ctx.cr6.lt) goto loc_8295E618;
	// rlwinm r9,r11,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 2) & 0xFFFFFFFC;
	// rlwinm r10,r11,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// add r11,r9,r30
	ctx.r11.u64 = ctx.r9.u64 + ctx.r30.u64;
loc_8295E5EC:
	// addi r9,r11,-4
	ctx.r9.s64 = ctx.r11.s64 + -4;
	// lfs f0,0(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// addi r10,r10,-1
	ctx.r10.s64 = ctx.r10.s64 + -1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// lfs f13,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// fadds f11,f13,f0
	ctx.f11.f64 = double(float(ctx.f13.f64 + ctx.f0.f64));
	// stfs f11,0(r11)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// fsubs f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// addi r11,r11,-8
	ctx.r11.s64 = ctx.r11.s64 + -8;
	// stfs f0,8(r9)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r9.u32 + 8, temp.u32);
	// bne cr6,0x8295e5ec
	if (!ctx.cr6.eq) goto loc_8295E5EC;
loc_8295E618:
	// lfs f0,0(r30)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// cmpwi cr6,r31,4
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 4, ctx.xer);
	// fsubs f13,f0,f12
	ctx.f13.f64 = double(float(ctx.f0.f64 - ctx.f12.f64));
	// stfs f13,4(r30)
	temp.f32 = float(ctx.f13.f64);
	REX_STORE_U32(ctx.r30.u32 + 4, temp.u32);
	// fadds f0,f0,f12
	ctx.f0.f64 = double(float(ctx.f0.f64 + ctx.f12.f64));
	// stfs f0,0(r30)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// ble cr6,0x8295e654
	if (!ctx.cr6.gt) goto loc_8295E654;
	// rlwinm r11,r29,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r5,r25
	ctx.r5.u64 = ctx.r25.u64;
	// add r6,r11,r28
	ctx.r6.u64 = ctx.r11.u64 + ctx.r28.u64;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295d120
	ctx.lr = 0x8295E64C;
	sub_8295D120(ctx, base);
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// b 0x8295e65c
	goto loc_8295E65C;
loc_8295E654:
	// bne cr6,0x8295e670
	if (!ctx.cr6.eq) goto loc_8295E670;
	// li r3,4
	ctx.r3.s64 = 4;
loc_8295E65C:
	// mr r7,r28
	ctx.r7.u64 = ctx.r28.u64;
	// mr r6,r29
	ctx.r6.u64 = ctx.r29.u64;
	// addi r5,r26,8
	ctx.r5.s64 = ctx.r26.s64 + 8;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// bl 0x8295e178
	ctx.lr = 0x8295E670;
	sub_8295E178(ctx, base);
loc_8295E670:
	// rlwinm r11,r29,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r29.u32 | (ctx.r29.u64 << 32), 2) & 0xFFFFFFFC;
	// mr r5,r25
	ctx.r5.u64 = ctx.r25.u64;
	// add r27,r11,r28
	ctx.r27.u64 = ctx.r11.u64 + ctx.r28.u64;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mr r6,r27
	ctx.r6.u64 = ctx.r27.u64;
	// bl 0x8295d1f0
	ctx.lr = 0x8295E68C;
	sub_8295D1F0(ctx, base);
	// cmpwi cr6,r24,0
	ctx.cr6.compare<int32_t>(ctx.r24.s32, 0, ctx.xer);
	// blt cr6,0x8295e748
	if (ctx.cr6.lt) goto loc_8295E748;
	// cmpwi cr6,r31,4
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 4, ctx.xer);
	// ble cr6,0x8295e6c8
	if (!ctx.cr6.gt) goto loc_8295E6C8;
	// mr r7,r28
	ctx.r7.u64 = ctx.r28.u64;
	// mr r6,r29
	ctx.r6.u64 = ctx.r29.u64;
	// addi r5,r26,8
	ctx.r5.s64 = ctx.r26.s64 + 8;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295de30
	ctx.lr = 0x8295E6B0;
	sub_8295DE30(ctx, base);
	// mr r6,r27
	ctx.r6.u64 = ctx.r27.u64;
	// mr r5,r25
	ctx.r5.u64 = ctx.r25.u64;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x8295d050
	ctx.lr = 0x8295E6C4;
	sub_8295D050(ctx, base);
	// b 0x8295e6e4
	goto loc_8295E6E4;
loc_8295E6C8:
	// bne cr6,0x8295e6e4
	if (!ctx.cr6.eq) goto loc_8295E6E4;
	// mr r7,r28
	ctx.r7.u64 = ctx.r28.u64;
	// mr r6,r29
	ctx.r6.u64 = ctx.r29.u64;
	// addi r5,r26,8
	ctx.r5.s64 = ctx.r26.s64 + 8;
	// mr r4,r30
	ctx.r4.u64 = ctx.r30.u64;
	// li r3,4
	ctx.r3.s64 = 4;
	// bl 0x8295de30
	ctx.lr = 0x8295E6E4;
	sub_8295DE30(ctx, base);
loc_8295E6E4:
	// lfs f0,0(r30)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// cmpwi cr6,r31,2
	ctx.cr6.compare<int32_t>(ctx.r31.s32, 2, ctx.xer);
	// lfs f13,4(r30)
	temp.u32 = REX_LOAD_U32(ctx.r30.u32 + 4);
	ctx.f13.f64 = double(temp.f32);
	// fadds f12,f13,f0
	ctx.f12.f64 = double(float(ctx.f13.f64 + ctx.f0.f64));
	// stfs f12,0(r30)
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r30.u32 + 0, temp.u32);
	// fsubs f12,f0,f13
	ctx.f12.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// ble cr6,0x8295e73c
	if (!ctx.cr6.gt) goto loc_8295E73C;
	// addi r10,r31,-3
	ctx.r10.s64 = ctx.r31.s64 + -3;
	// addi r11,r30,8
	ctx.r11.s64 = ctx.r30.s64 + 8;
	// rlwinm r10,r10,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
loc_8295E710:
	// addi r9,r11,4
	ctx.r9.s64 = ctx.r11.s64 + 4;
	// lfs f0,0(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + 0);
	ctx.f0.f64 = double(temp.f32);
	// addi r10,r10,-1
	ctx.r10.s64 = ctx.r10.s64 + -1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// lfs f13,0(r9)
	temp.u32 = REX_LOAD_U32(ctx.r9.u32 + 0);
	ctx.f13.f64 = double(temp.f32);
	// fadds f11,f13,f0
	ctx.f11.f64 = double(float(ctx.f13.f64 + ctx.f0.f64));
	// stfs f11,0(r11)
	temp.f32 = float(ctx.f11.f64);
	REX_STORE_U32(ctx.r11.u32 + 0, temp.u32);
	// fsubs f0,f0,f13
	ctx.f0.f64 = double(float(ctx.f0.f64 - ctx.f13.f64));
	// addi r11,r11,8
	ctx.r11.s64 = ctx.r11.s64 + 8;
	// stfs f0,-8(r9)
	temp.f32 = float(ctx.f0.f64);
	REX_STORE_U32(ctx.r9.u32 + -8, temp.u32);
	// bne cr6,0x8295e710
	if (!ctx.cr6.eq) goto loc_8295E710;
loc_8295E73C:
	// rlwinm r11,r31,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 2) & 0xFFFFFFFC;
	// add r11,r11,r30
	ctx.r11.u64 = ctx.r11.u64 + ctx.r30.u64;
	// stfs f12,-4(r11)
	ctx.fpscr.disableFlushMode();
	temp.f32 = float(ctx.f12.f64);
	REX_STORE_U32(ctx.r11.u32 + -4, temp.u32);
loc_8295E748:
	// addi r1,r1,160
	ctx.r1.s64 = ctx.r1.s64 + 160;
	// b 0x824131e8
	__restgprlr_24(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295E750) {
	REX_FUNC_PROLOGUE(sub_8295E750);
	// clrlwi r11,r4,16
	ctx.r11.u64 = ctx.r4.u32 & 0xFFFF;
	// rlwinm r10,r5,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 31) & 0x7FFFFFFF;
	// rlwinm r8,r11,16,0,15
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 16) & 0xFFFF0000;
	// mr r9,r3
	ctx.r9.u64 = ctx.r3.u64;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// or r8,r8,r11
	ctx.r8.u64 = ctx.r8.u64 | ctx.r11.u64;
	// beq cr6,0x8295e788
	if (ctx.cr6.eq) goto loc_8295E788;
	// mr r11,r3
	ctx.r11.u64 = ctx.r3.u64;
	// mtctr r10
	ctx.ctr.u64 = ctx.r10.u64;
loc_8295E774:
	// stw r8,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r8.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// bdnz 0x8295e774
	--ctx.ctr.u64;
	if (ctx.ctr.u32 != 0) goto loc_8295E774;
	// rlwinm r11,r10,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 2) & 0xFFFFFFFC;
	// add r9,r11,r3
	ctx.r9.u64 = ctx.r11.u64 + ctx.r3.u64;
loc_8295E788:
	// clrlwi r11,r5,31
	ctx.r11.u64 = ctx.r5.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beqlr cr6
	if (ctx.cr6.eq) return;
	// sth r8,0(r9)
	REX_STORE_U16(ctx.r9.u32 + 0, ctx.r8.u16);
	// blr 
	return;
}

DEFINE_REX_FUNC(sub_8295E7A0) {
	REX_FUNC_PROLOGUE(sub_8295E7A0);
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413188
	ctx.lr = 0x8295E7A8;
	__savegprlr_20(ctx, base);
	// mr r10,r5
	ctx.r10.u64 = ctx.r5.u64;
	// addi r11,r1,-368
	ctx.r11.s64 = ctx.r1.s64 + -368;
	// li r25,8
	ctx.r25.s64 = 8;
loc_8295E7B4:
	// lhz r7,48(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 48);
	// lhz r8,16(r10)
	ctx.r8.u64 = REX_LOAD_U16(ctx.r10.u32 + 16);
	// lhz r9,32(r10)
	ctx.r9.u64 = REX_LOAD_U16(ctx.r10.u32 + 32);
	// extsh r30,r7
	ctx.r30.s64 = ctx.r7.s16;
	// lhz r7,80(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 80);
	// extsh r31,r8
	ctx.r31.s64 = ctx.r8.s16;
	// lhz r5,96(r10)
	ctx.r5.u64 = REX_LOAD_U16(ctx.r10.u32 + 96);
	// extsh r9,r9
	ctx.r9.s64 = ctx.r9.s16;
	// extsh r29,r7
	ctx.r29.s64 = ctx.r7.s16;
	// lhz r8,64(r10)
	ctx.r8.u64 = REX_LOAD_U16(ctx.r10.u32 + 64);
	// extsh r7,r5
	ctx.r7.s64 = ctx.r5.s16;
	// lhz r28,112(r10)
	ctx.r28.u64 = REX_LOAD_U16(ctx.r10.u32 + 112);
	// or r5,r31,r9
	ctx.r5.u64 = ctx.r31.u64 | ctx.r9.u64;
	// extsh r8,r8
	ctx.r8.s64 = ctx.r8.s16;
	// or r5,r5,r30
	ctx.r5.u64 = ctx.r5.u64 | ctx.r30.u64;
	// extsh r28,r28
	ctx.r28.s64 = ctx.r28.s16;
	// or r5,r5,r8
	ctx.r5.u64 = ctx.r5.u64 | ctx.r8.u64;
	// or r5,r5,r29
	ctx.r5.u64 = ctx.r5.u64 | ctx.r29.u64;
	// or r5,r5,r7
	ctx.r5.u64 = ctx.r5.u64 | ctx.r7.u64;
	// or r5,r5,r28
	ctx.r5.u64 = ctx.r5.u64 | ctx.r28.u64;
	// extsh r5,r5
	ctx.r5.s64 = ctx.r5.s16;
	// cmpwi cr6,r5,0
	ctx.cr6.compare<int32_t>(ctx.r5.s32, 0, ctx.xer);
	// bne cr6,0x8295e84c
	if (!ctx.cr6.eq) goto loc_8295E84C;
	// lhz r9,0(r10)
	ctx.r9.u64 = REX_LOAD_U16(ctx.r10.u32 + 0);
	// addi r10,r10,2
	ctx.r10.s64 = ctx.r10.s64 + 2;
	// lwz r8,0(r6)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// addi r6,r6,4
	ctx.r6.s64 = ctx.r6.s64 + 4;
	// extsh r9,r9
	ctx.r9.s64 = ctx.r9.s16;
	// mullw r9,r9,r8
	ctx.r9.s64 = int64_t(ctx.r9.s32) * int64_t(ctx.r8.s32);
	// srawi r9,r9,11
	ctx.xer.ca = (ctx.r9.s32 < 0) & ((ctx.r9.u32 & 0x7FF) != 0);
	ctx.r9.s64 = ctx.r9.s32 >> 11;
	// stw r9,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r9.u32);
	// stw r9,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r9.u32);
	// stw r9,64(r11)
	REX_STORE_U32(ctx.r11.u32 + 64, ctx.r9.u32);
	// stw r9,128(r11)
	REX_STORE_U32(ctx.r11.u32 + 128, ctx.r9.u32);
	// stw r9,160(r11)
	REX_STORE_U32(ctx.r11.u32 + 160, ctx.r9.u32);
	// stw r9,192(r11)
	REX_STORE_U32(ctx.r11.u32 + 192, ctx.r9.u32);
	// stw r9,224(r11)
	REX_STORE_U32(ctx.r11.u32 + 224, ctx.r9.u32);
	// b 0x8295e974
	goto loc_8295E974;
loc_8295E84C:
	// lhz r5,0(r10)
	ctx.r5.u64 = REX_LOAD_U16(ctx.r10.u32 + 0);
	// addi r10,r10,2
	ctx.r10.s64 = ctx.r10.s64 + 2;
	// lwz r27,0(r6)
	ctx.r27.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// extsh r5,r5
	ctx.r5.s64 = ctx.r5.s16;
	// lwz r26,64(r6)
	ctx.r26.u64 = REX_LOAD_U32(ctx.r6.u32 + 64);
	// lwz r24,128(r6)
	ctx.r24.u64 = REX_LOAD_U32(ctx.r6.u32 + 128);
	// mullw r5,r5,r27
	ctx.r5.s64 = int64_t(ctx.r5.s32) * int64_t(ctx.r27.s32);
	// lwz r23,192(r6)
	ctx.r23.u64 = REX_LOAD_U32(ctx.r6.u32 + 192);
	// lwz r27,32(r6)
	ctx.r27.u64 = REX_LOAD_U32(ctx.r6.u32 + 32);
	// lwz r22,96(r6)
	ctx.r22.u64 = REX_LOAD_U32(ctx.r6.u32 + 96);
	// lwz r21,160(r6)
	ctx.r21.u64 = REX_LOAD_U32(ctx.r6.u32 + 160);
	// lwz r20,224(r6)
	ctx.r20.u64 = REX_LOAD_U32(ctx.r6.u32 + 224);
	// mullw r9,r26,r9
	ctx.r9.s64 = int64_t(ctx.r26.s32) * int64_t(ctx.r9.s32);
	// mullw r26,r24,r8
	ctx.r26.s64 = int64_t(ctx.r24.s32) * int64_t(ctx.r8.s32);
	// srawi r8,r5,11
	ctx.xer.ca = (ctx.r5.s32 < 0) & ((ctx.r5.u32 & 0x7FF) != 0);
	ctx.r8.s64 = ctx.r5.s32 >> 11;
	// mullw r5,r23,r7
	ctx.r5.s64 = int64_t(ctx.r23.s32) * int64_t(ctx.r7.s32);
	// srawi r9,r9,11
	ctx.xer.ca = (ctx.r9.s32 < 0) & ((ctx.r9.u32 & 0x7FF) != 0);
	ctx.r9.s64 = ctx.r9.s32 >> 11;
	// srawi r7,r26,11
	ctx.xer.ca = (ctx.r26.s32 < 0) & ((ctx.r26.u32 & 0x7FF) != 0);
	ctx.r7.s64 = ctx.r26.s32 >> 11;
	// srawi r5,r5,11
	ctx.xer.ca = (ctx.r5.s32 < 0) & ((ctx.r5.u32 & 0x7FF) != 0);
	ctx.r5.s64 = ctx.r5.s32 >> 11;
	// mullw r27,r27,r31
	ctx.r27.s64 = int64_t(ctx.r27.s32) * int64_t(ctx.r31.s32);
	// subf r26,r5,r9
	ctx.r26.u64 = ctx.r9.u64 - ctx.r5.u64;
	// add r9,r5,r9
	ctx.r9.u64 = ctx.r5.u64 + ctx.r9.u64;
	// mulli r5,r26,2896
	ctx.r5.s64 = static_cast<int64_t>(ctx.r26.u64 * static_cast<uint64_t>(2896));
	// mullw r30,r22,r30
	ctx.r30.s64 = int64_t(ctx.r22.s32) * int64_t(ctx.r30.s32);
	// srawi r26,r5,11
	ctx.xer.ca = (ctx.r5.s32 < 0) & ((ctx.r5.u32 & 0x7FF) != 0);
	ctx.r26.s64 = ctx.r5.s32 >> 11;
	// mullw r29,r21,r29
	ctx.r29.s64 = int64_t(ctx.r21.s32) * int64_t(ctx.r29.s32);
	// add r31,r7,r8
	ctx.r31.u64 = ctx.r7.u64 + ctx.r8.u64;
	// mullw r28,r20,r28
	ctx.r28.s64 = int64_t(ctx.r20.s32) * int64_t(ctx.r28.s32);
	// srawi r5,r27,11
	ctx.xer.ca = (ctx.r27.s32 < 0) & ((ctx.r27.u32 & 0x7FF) != 0);
	ctx.r5.s64 = ctx.r27.s32 >> 11;
	// srawi r30,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r30.s64 = ctx.r30.s32 >> 11;
	// srawi r29,r29,11
	ctx.xer.ca = (ctx.r29.s32 < 0) & ((ctx.r29.u32 & 0x7FF) != 0);
	ctx.r29.s64 = ctx.r29.s32 >> 11;
	// subf r8,r7,r8
	ctx.r8.u64 = ctx.r8.u64 - ctx.r7.u64;
	// add r27,r9,r31
	ctx.r27.u64 = ctx.r9.u64 + ctx.r31.u64;
	// srawi r28,r28,11
	ctx.xer.ca = (ctx.r28.s32 < 0) & ((ctx.r28.u32 & 0x7FF) != 0);
	ctx.r28.s64 = ctx.r28.s32 >> 11;
	// subf r7,r9,r26
	ctx.r7.u64 = ctx.r26.u64 - ctx.r9.u64;
	// subf r31,r9,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r9.u64;
	// subf r9,r30,r29
	ctx.r9.u64 = ctx.r29.u64 - ctx.r30.u64;
	// subf r26,r28,r5
	ctx.r26.u64 = ctx.r5.u64 - ctx.r28.u64;
	// add r30,r29,r30
	ctx.r30.u64 = ctx.r29.u64 + ctx.r30.u64;
	// add r29,r7,r8
	ctx.r29.u64 = ctx.r7.u64 + ctx.r8.u64;
	// add r5,r28,r5
	ctx.r5.u64 = ctx.r28.u64 + ctx.r5.u64;
	// subf r7,r7,r8
	ctx.r7.u64 = ctx.r8.u64 - ctx.r7.u64;
	// add r8,r26,r9
	ctx.r8.u64 = ctx.r26.u64 + ctx.r9.u64;
	// mulli r28,r9,-5352
	ctx.r28.s64 = static_cast<int64_t>(ctx.r9.u64 * static_cast<uint64_t>(-5352));
	// add r9,r5,r30
	ctx.r9.u64 = ctx.r5.u64 + ctx.r30.u64;
	// subf r5,r30,r5
	ctx.r5.u64 = ctx.r5.u64 - ctx.r30.u64;
	// mulli r8,r8,3784
	ctx.r8.s64 = static_cast<int64_t>(ctx.r8.u64 * static_cast<uint64_t>(3784));
	// srawi r8,r8,11
	ctx.xer.ca = (ctx.r8.s32 < 0) & ((ctx.r8.u32 & 0x7FF) != 0);
	ctx.r8.s64 = ctx.r8.s32 >> 11;
	// mulli r5,r5,2896
	ctx.r5.s64 = static_cast<int64_t>(ctx.r5.u64 * static_cast<uint64_t>(2896));
	// srawi r28,r28,11
	ctx.xer.ca = (ctx.r28.s32 < 0) & ((ctx.r28.u32 & 0x7FF) != 0);
	ctx.r28.s64 = ctx.r28.s32 >> 11;
	// mulli r30,r26,2217
	ctx.r30.s64 = static_cast<int64_t>(ctx.r26.u64 * static_cast<uint64_t>(2217));
	// srawi r26,r5,11
	ctx.xer.ca = (ctx.r5.s32 < 0) & ((ctx.r5.u32 & 0x7FF) != 0);
	ctx.r26.s64 = ctx.r5.s32 >> 11;
	// subf r5,r9,r28
	ctx.r5.u64 = ctx.r28.u64 - ctx.r9.u64;
	// add r28,r9,r27
	ctx.r28.u64 = ctx.r9.u64 + ctx.r27.u64;
	// subf r9,r9,r27
	ctx.r9.u64 = ctx.r27.u64 - ctx.r9.u64;
	// srawi r30,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r30.s64 = ctx.r30.s32 >> 11;
	// addi r6,r6,4
	ctx.r6.s64 = ctx.r6.s64 + 4;
	// stw r28,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r28.u32);
	// stw r9,224(r11)
	REX_STORE_U32(ctx.r11.u32 + 224, ctx.r9.u32);
	// add r9,r5,r8
	ctx.r9.u64 = ctx.r5.u64 + ctx.r8.u64;
	// subf r5,r8,r30
	ctx.r5.u64 = ctx.r30.u64 - ctx.r8.u64;
	// subf r8,r9,r26
	ctx.r8.u64 = ctx.r26.u64 - ctx.r9.u64;
	// add r30,r9,r29
	ctx.r30.u64 = ctx.r9.u64 + ctx.r29.u64;
	// subf r9,r9,r29
	ctx.r9.u64 = ctx.r29.u64 - ctx.r9.u64;
	// stw r30,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r30.u32);
	// stw r9,192(r11)
	REX_STORE_U32(ctx.r11.u32 + 192, ctx.r9.u32);
	// add r9,r5,r8
	ctx.r9.u64 = ctx.r5.u64 + ctx.r8.u64;
	// add r5,r8,r7
	ctx.r5.u64 = ctx.r8.u64 + ctx.r7.u64;
	// subf r8,r8,r7
	ctx.r8.u64 = ctx.r7.u64 - ctx.r8.u64;
	// stw r5,64(r11)
	REX_STORE_U32(ctx.r11.u32 + 64, ctx.r5.u32);
	// stw r8,160(r11)
	REX_STORE_U32(ctx.r11.u32 + 160, ctx.r8.u32);
	// add r8,r9,r31
	ctx.r8.u64 = ctx.r9.u64 + ctx.r31.u64;
	// subf r9,r9,r31
	ctx.r9.u64 = ctx.r31.u64 - ctx.r9.u64;
	// stw r8,128(r11)
	REX_STORE_U32(ctx.r11.u32 + 128, ctx.r8.u32);
loc_8295E974:
	// addi r25,r25,-1
	ctx.r25.s64 = ctx.r25.s64 + -1;
	// stw r9,96(r11)
	REX_STORE_U32(ctx.r11.u32 + 96, ctx.r9.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// cmpwi cr6,r25,0
	ctx.cr6.compare<int32_t>(ctx.r25.s32, 0, ctx.xer);
	// bgt cr6,0x8295e7b4
	if (ctx.cr6.gt) goto loc_8295E7B4;
	// addi r10,r3,1
	ctx.r10.s64 = ctx.r3.s64 + 1;
	// addi r11,r1,-344
	ctx.r11.s64 = ctx.r1.s64 + -344;
	// li r7,8
	ctx.r7.s64 = 8;
loc_8295E994:
	// lwz r9,-4(r11)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r11.u32 + -4);
	// addi r7,r7,-1
	ctx.r7.s64 = ctx.r7.s64 + -1;
	// lwz r8,-12(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + -12);
	// lwz r5,-20(r11)
	ctx.r5.u64 = REX_LOAD_U32(ctx.r11.u32 + -20);
	// cmplwi cr6,r7,0
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 0, ctx.xer);
	// lwz r6,4(r11)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r11.u32 + 4);
	// subf r28,r8,r9
	ctx.r28.u64 = ctx.r9.u64 - ctx.r8.u64;
	// lwz r3,0(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// add r27,r8,r9
	ctx.r27.u64 = ctx.r8.u64 + ctx.r9.u64;
	// lwz r31,-16(r11)
	ctx.r31.u64 = REX_LOAD_U32(ctx.r11.u32 + -16);
	// subf r26,r6,r5
	ctx.r26.u64 = ctx.r5.u64 - ctx.r6.u64;
	// lwz r30,-8(r11)
	ctx.r30.u64 = REX_LOAD_U32(ctx.r11.u32 + -8);
	// add r6,r5,r6
	ctx.r6.u64 = ctx.r5.u64 + ctx.r6.u64;
	// add r9,r3,r31
	ctx.r9.u64 = ctx.r3.u64 + ctx.r31.u64;
	// lwz r29,-24(r11)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r11.u32 + -24);
	// subf r8,r3,r31
	ctx.r8.u64 = ctx.r31.u64 - ctx.r3.u64;
	// add r31,r26,r28
	ctx.r31.u64 = ctx.r26.u64 + ctx.r28.u64;
	// mulli r8,r8,2896
	ctx.r8.s64 = static_cast<int64_t>(ctx.r8.u64 * static_cast<uint64_t>(2896));
	// subf r3,r30,r29
	ctx.r3.u64 = ctx.r29.u64 - ctx.r30.u64;
	// add r5,r29,r30
	ctx.r5.u64 = ctx.r29.u64 + ctx.r30.u64;
	// mulli r31,r31,3784
	ctx.r31.s64 = static_cast<int64_t>(ctx.r31.u64 * static_cast<uint64_t>(3784));
	// mulli r30,r28,-5352
	ctx.r30.s64 = static_cast<int64_t>(ctx.r28.u64 * static_cast<uint64_t>(-5352));
	// srawi r25,r8,11
	ctx.xer.ca = (ctx.r8.s32 < 0) & ((ctx.r8.u32 & 0x7FF) != 0);
	ctx.r25.s64 = ctx.r8.s32 >> 11;
	// srawi r31,r31,11
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x7FF) != 0);
	ctx.r31.s64 = ctx.r31.s32 >> 11;
	// srawi r29,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r29.s64 = ctx.r30.s32 >> 11;
	// subf r30,r27,r6
	ctx.r30.u64 = ctx.r6.u64 - ctx.r27.u64;
	// add r8,r6,r27
	ctx.r8.u64 = ctx.r6.u64 + ctx.r27.u64;
	// mulli r30,r30,2896
	ctx.r30.s64 = static_cast<int64_t>(ctx.r30.u64 * static_cast<uint64_t>(2896));
	// srawi r28,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r28.s64 = ctx.r30.s32 >> 11;
	// mulli r27,r26,2217
	ctx.r27.s64 = static_cast<int64_t>(ctx.r26.u64 * static_cast<uint64_t>(2217));
	// add r30,r9,r5
	ctx.r30.u64 = ctx.r9.u64 + ctx.r5.u64;
	// subf r6,r9,r25
	ctx.r6.u64 = ctx.r25.u64 - ctx.r9.u64;
	// subf r5,r9,r5
	ctx.r5.u64 = ctx.r5.u64 - ctx.r9.u64;
	// subf r9,r8,r29
	ctx.r9.u64 = ctx.r29.u64 - ctx.r8.u64;
	// srawi r27,r27,11
	ctx.xer.ca = (ctx.r27.s32 < 0) & ((ctx.r27.u32 & 0x7FF) != 0);
	ctx.r27.s64 = ctx.r27.s32 >> 11;
	// add r9,r9,r31
	ctx.r9.u64 = ctx.r9.u64 + ctx.r31.u64;
	// subf r29,r31,r27
	ctx.r29.u64 = ctx.r27.u64 - ctx.r31.u64;
	// add r31,r6,r3
	ctx.r31.u64 = ctx.r6.u64 + ctx.r3.u64;
	// subf r6,r6,r3
	ctx.r6.u64 = ctx.r3.u64 - ctx.r6.u64;
	// add r3,r8,r30
	ctx.r3.u64 = ctx.r8.u64 + ctx.r30.u64;
	// subf r8,r8,r30
	ctx.r8.u64 = ctx.r30.u64 - ctx.r8.u64;
	// addi r3,r3,127
	ctx.r3.s64 = ctx.r3.s64 + 127;
	// addi r8,r8,127
	ctx.r8.s64 = ctx.r8.s64 + 127;
	// srawi r3,r3,8
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0xFF) != 0);
	ctx.r3.s64 = ctx.r3.s32 >> 8;
	// srawi r8,r8,8
	ctx.xer.ca = (ctx.r8.s32 < 0) & ((ctx.r8.u32 & 0xFF) != 0);
	ctx.r8.s64 = ctx.r8.s32 >> 8;
	// addi r11,r11,32
	ctx.r11.s64 = ctx.r11.s64 + 32;
	// stb r3,-1(r10)
	REX_STORE_U8(ctx.r10.u32 + -1, ctx.r3.u8);
	// add r3,r9,r31
	ctx.r3.u64 = ctx.r9.u64 + ctx.r31.u64;
	// stb r8,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r8.u8);
	// subf r8,r9,r28
	ctx.r8.u64 = ctx.r28.u64 - ctx.r9.u64;
	// subf r9,r9,r31
	ctx.r9.u64 = ctx.r31.u64 - ctx.r9.u64;
	// addi r3,r3,127
	ctx.r3.s64 = ctx.r3.s64 + 127;
	// addi r9,r9,127
	ctx.r9.s64 = ctx.r9.s64 + 127;
	// srawi r3,r3,8
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0xFF) != 0);
	ctx.r3.s64 = ctx.r3.s32 >> 8;
	// srawi r9,r9,8
	ctx.xer.ca = (ctx.r9.s32 < 0) & ((ctx.r9.u32 & 0xFF) != 0);
	ctx.r9.s64 = ctx.r9.s32 >> 8;
	// stb r3,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r3.u8);
	// add r3,r8,r6
	ctx.r3.u64 = ctx.r8.u64 + ctx.r6.u64;
	// stb r9,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r9.u8);
	// add r9,r29,r8
	ctx.r9.u64 = ctx.r29.u64 + ctx.r8.u64;
	// subf r8,r8,r6
	ctx.r8.u64 = ctx.r6.u64 - ctx.r8.u64;
	// addi r6,r3,127
	ctx.r6.s64 = ctx.r3.s64 + 127;
	// addi r8,r8,127
	ctx.r8.s64 = ctx.r8.s64 + 127;
	// srawi r6,r6,8
	ctx.xer.ca = (ctx.r6.s32 < 0) & ((ctx.r6.u32 & 0xFF) != 0);
	ctx.r6.s64 = ctx.r6.s32 >> 8;
	// srawi r8,r8,8
	ctx.xer.ca = (ctx.r8.s32 < 0) & ((ctx.r8.u32 & 0xFF) != 0);
	ctx.r8.s64 = ctx.r8.s32 >> 8;
	// stb r6,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r6.u8);
	// stb r8,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r8.u8);
	// add r8,r9,r5
	ctx.r8.u64 = ctx.r9.u64 + ctx.r5.u64;
	// subf r9,r9,r5
	ctx.r9.u64 = ctx.r5.u64 - ctx.r9.u64;
	// addi r8,r8,127
	ctx.r8.s64 = ctx.r8.s64 + 127;
	// addi r9,r9,127
	ctx.r9.s64 = ctx.r9.s64 + 127;
	// srawi r8,r8,8
	ctx.xer.ca = (ctx.r8.s32 < 0) & ((ctx.r8.u32 & 0xFF) != 0);
	ctx.r8.s64 = ctx.r8.s32 >> 8;
	// srawi r9,r9,8
	ctx.xer.ca = (ctx.r9.s32 < 0) & ((ctx.r9.u32 & 0xFF) != 0);
	ctx.r9.s64 = ctx.r9.s32 >> 8;
	// stb r8,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r8.u8);
	// stb r9,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r9.u8);
	// add r10,r10,r4
	ctx.r10.u64 = ctx.r10.u64 + ctx.r4.u64;
	// bne cr6,0x8295e994
	if (!ctx.cr6.eq) goto loc_8295E994;
	// b 0x824131d8
	__restgprlr_20(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295EAC8) {
	REX_FUNC_PROLOGUE(sub_8295EAC8);
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413184
	ctx.lr = 0x8295EAD0;
	__savegprlr_19(ctx, base);
	// mr r10,r5
	ctx.r10.u64 = ctx.r5.u64;
	// add r9,r3,r4
	ctx.r9.u64 = ctx.r3.u64 + ctx.r4.u64;
	// rlwinm r25,r4,1,0,30
	ctx.r25.u64 = __builtin_rotateleft64(ctx.r4.u32 | (ctx.r4.u64 << 32), 1) & 0xFFFFFFFE;
	// addi r11,r1,-368
	ctx.r11.s64 = ctx.r1.s64 + -368;
	// li r24,8
	ctx.r24.s64 = 8;
loc_8295EAE4:
	// lhz r5,48(r10)
	ctx.r5.u64 = REX_LOAD_U16(ctx.r10.u32 + 48);
	// lhz r7,16(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 16);
	// lhz r8,32(r10)
	ctx.r8.u64 = REX_LOAD_U16(ctx.r10.u32 + 32);
	// extsh r30,r5
	ctx.r30.s64 = ctx.r5.s16;
	// lhz r5,80(r10)
	ctx.r5.u64 = REX_LOAD_U16(ctx.r10.u32 + 80);
	// extsh r31,r7
	ctx.r31.s64 = ctx.r7.s16;
	// lhz r4,96(r10)
	ctx.r4.u64 = REX_LOAD_U16(ctx.r10.u32 + 96);
	// extsh r8,r8
	ctx.r8.s64 = ctx.r8.s16;
	// extsh r29,r5
	ctx.r29.s64 = ctx.r5.s16;
	// lhz r7,64(r10)
	ctx.r7.u64 = REX_LOAD_U16(ctx.r10.u32 + 64);
	// extsh r5,r4
	ctx.r5.s64 = ctx.r4.s16;
	// lhz r28,112(r10)
	ctx.r28.u64 = REX_LOAD_U16(ctx.r10.u32 + 112);
	// or r4,r31,r8
	ctx.r4.u64 = ctx.r31.u64 | ctx.r8.u64;
	// extsh r7,r7
	ctx.r7.s64 = ctx.r7.s16;
	// or r4,r4,r30
	ctx.r4.u64 = ctx.r4.u64 | ctx.r30.u64;
	// extsh r28,r28
	ctx.r28.s64 = ctx.r28.s16;
	// or r4,r4,r7
	ctx.r4.u64 = ctx.r4.u64 | ctx.r7.u64;
	// or r4,r4,r29
	ctx.r4.u64 = ctx.r4.u64 | ctx.r29.u64;
	// or r4,r4,r5
	ctx.r4.u64 = ctx.r4.u64 | ctx.r5.u64;
	// or r4,r4,r28
	ctx.r4.u64 = ctx.r4.u64 | ctx.r28.u64;
	// extsh r4,r4
	ctx.r4.s64 = ctx.r4.s16;
	// cmpwi cr6,r4,0
	ctx.cr6.compare<int32_t>(ctx.r4.s32, 0, ctx.xer);
	// bne cr6,0x8295eb7c
	if (!ctx.cr6.eq) goto loc_8295EB7C;
	// lhz r8,0(r10)
	ctx.r8.u64 = REX_LOAD_U16(ctx.r10.u32 + 0);
	// addi r10,r10,2
	ctx.r10.s64 = ctx.r10.s64 + 2;
	// lwz r7,0(r6)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// addi r6,r6,4
	ctx.r6.s64 = ctx.r6.s64 + 4;
	// extsh r8,r8
	ctx.r8.s64 = ctx.r8.s16;
	// mullw r8,r8,r7
	ctx.r8.s64 = int64_t(ctx.r8.s32) * int64_t(ctx.r7.s32);
	// srawi r8,r8,11
	ctx.xer.ca = (ctx.r8.s32 < 0) & ((ctx.r8.u32 & 0x7FF) != 0);
	ctx.r8.s64 = ctx.r8.s32 >> 11;
	// stw r8,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r8.u32);
	// stw r8,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r8.u32);
	// stw r8,64(r11)
	REX_STORE_U32(ctx.r11.u32 + 64, ctx.r8.u32);
	// stw r8,128(r11)
	REX_STORE_U32(ctx.r11.u32 + 128, ctx.r8.u32);
	// stw r8,160(r11)
	REX_STORE_U32(ctx.r11.u32 + 160, ctx.r8.u32);
	// stw r8,192(r11)
	REX_STORE_U32(ctx.r11.u32 + 192, ctx.r8.u32);
	// stw r8,224(r11)
	REX_STORE_U32(ctx.r11.u32 + 224, ctx.r8.u32);
	// b 0x8295eca4
	goto loc_8295ECA4;
loc_8295EB7C:
	// lhz r4,0(r10)
	ctx.r4.u64 = REX_LOAD_U16(ctx.r10.u32 + 0);
	// addi r10,r10,2
	ctx.r10.s64 = ctx.r10.s64 + 2;
	// lwz r27,0(r6)
	ctx.r27.u64 = REX_LOAD_U32(ctx.r6.u32 + 0);
	// extsh r4,r4
	ctx.r4.s64 = ctx.r4.s16;
	// lwz r26,64(r6)
	ctx.r26.u64 = REX_LOAD_U32(ctx.r6.u32 + 64);
	// lwz r23,128(r6)
	ctx.r23.u64 = REX_LOAD_U32(ctx.r6.u32 + 128);
	// mullw r4,r4,r27
	ctx.r4.s64 = int64_t(ctx.r4.s32) * int64_t(ctx.r27.s32);
	// lwz r22,192(r6)
	ctx.r22.u64 = REX_LOAD_U32(ctx.r6.u32 + 192);
	// lwz r27,32(r6)
	ctx.r27.u64 = REX_LOAD_U32(ctx.r6.u32 + 32);
	// lwz r21,96(r6)
	ctx.r21.u64 = REX_LOAD_U32(ctx.r6.u32 + 96);
	// lwz r20,160(r6)
	ctx.r20.u64 = REX_LOAD_U32(ctx.r6.u32 + 160);
	// lwz r19,224(r6)
	ctx.r19.u64 = REX_LOAD_U32(ctx.r6.u32 + 224);
	// mullw r8,r26,r8
	ctx.r8.s64 = int64_t(ctx.r26.s32) * int64_t(ctx.r8.s32);
	// mullw r26,r23,r7
	ctx.r26.s64 = int64_t(ctx.r23.s32) * int64_t(ctx.r7.s32);
	// srawi r7,r4,11
	ctx.xer.ca = (ctx.r4.s32 < 0) & ((ctx.r4.u32 & 0x7FF) != 0);
	ctx.r7.s64 = ctx.r4.s32 >> 11;
	// mullw r4,r22,r5
	ctx.r4.s64 = int64_t(ctx.r22.s32) * int64_t(ctx.r5.s32);
	// srawi r8,r8,11
	ctx.xer.ca = (ctx.r8.s32 < 0) & ((ctx.r8.u32 & 0x7FF) != 0);
	ctx.r8.s64 = ctx.r8.s32 >> 11;
	// srawi r5,r26,11
	ctx.xer.ca = (ctx.r26.s32 < 0) & ((ctx.r26.u32 & 0x7FF) != 0);
	ctx.r5.s64 = ctx.r26.s32 >> 11;
	// srawi r4,r4,11
	ctx.xer.ca = (ctx.r4.s32 < 0) & ((ctx.r4.u32 & 0x7FF) != 0);
	ctx.r4.s64 = ctx.r4.s32 >> 11;
	// mullw r27,r27,r31
	ctx.r27.s64 = int64_t(ctx.r27.s32) * int64_t(ctx.r31.s32);
	// subf r26,r4,r8
	ctx.r26.u64 = ctx.r8.u64 - ctx.r4.u64;
	// add r8,r4,r8
	ctx.r8.u64 = ctx.r4.u64 + ctx.r8.u64;
	// mulli r4,r26,2896
	ctx.r4.s64 = static_cast<int64_t>(ctx.r26.u64 * static_cast<uint64_t>(2896));
	// mullw r30,r21,r30
	ctx.r30.s64 = int64_t(ctx.r21.s32) * int64_t(ctx.r30.s32);
	// srawi r26,r4,11
	ctx.xer.ca = (ctx.r4.s32 < 0) & ((ctx.r4.u32 & 0x7FF) != 0);
	ctx.r26.s64 = ctx.r4.s32 >> 11;
	// mullw r29,r20,r29
	ctx.r29.s64 = int64_t(ctx.r20.s32) * int64_t(ctx.r29.s32);
	// add r31,r5,r7
	ctx.r31.u64 = ctx.r5.u64 + ctx.r7.u64;
	// mullw r28,r19,r28
	ctx.r28.s64 = int64_t(ctx.r19.s32) * int64_t(ctx.r28.s32);
	// srawi r4,r27,11
	ctx.xer.ca = (ctx.r27.s32 < 0) & ((ctx.r27.u32 & 0x7FF) != 0);
	ctx.r4.s64 = ctx.r27.s32 >> 11;
	// srawi r30,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r30.s64 = ctx.r30.s32 >> 11;
	// srawi r29,r29,11
	ctx.xer.ca = (ctx.r29.s32 < 0) & ((ctx.r29.u32 & 0x7FF) != 0);
	ctx.r29.s64 = ctx.r29.s32 >> 11;
	// subf r7,r5,r7
	ctx.r7.u64 = ctx.r7.u64 - ctx.r5.u64;
	// add r27,r8,r31
	ctx.r27.u64 = ctx.r8.u64 + ctx.r31.u64;
	// srawi r28,r28,11
	ctx.xer.ca = (ctx.r28.s32 < 0) & ((ctx.r28.u32 & 0x7FF) != 0);
	ctx.r28.s64 = ctx.r28.s32 >> 11;
	// subf r5,r8,r26
	ctx.r5.u64 = ctx.r26.u64 - ctx.r8.u64;
	// subf r31,r8,r31
	ctx.r31.u64 = ctx.r31.u64 - ctx.r8.u64;
	// subf r8,r30,r29
	ctx.r8.u64 = ctx.r29.u64 - ctx.r30.u64;
	// subf r26,r28,r4
	ctx.r26.u64 = ctx.r4.u64 - ctx.r28.u64;
	// add r30,r29,r30
	ctx.r30.u64 = ctx.r29.u64 + ctx.r30.u64;
	// add r29,r5,r7
	ctx.r29.u64 = ctx.r5.u64 + ctx.r7.u64;
	// add r4,r28,r4
	ctx.r4.u64 = ctx.r28.u64 + ctx.r4.u64;
	// subf r5,r5,r7
	ctx.r5.u64 = ctx.r7.u64 - ctx.r5.u64;
	// add r7,r26,r8
	ctx.r7.u64 = ctx.r26.u64 + ctx.r8.u64;
	// mulli r28,r8,-5352
	ctx.r28.s64 = static_cast<int64_t>(ctx.r8.u64 * static_cast<uint64_t>(-5352));
	// add r8,r4,r30
	ctx.r8.u64 = ctx.r4.u64 + ctx.r30.u64;
	// subf r4,r30,r4
	ctx.r4.u64 = ctx.r4.u64 - ctx.r30.u64;
	// mulli r7,r7,3784
	ctx.r7.s64 = static_cast<int64_t>(ctx.r7.u64 * static_cast<uint64_t>(3784));
	// srawi r7,r7,11
	ctx.xer.ca = (ctx.r7.s32 < 0) & ((ctx.r7.u32 & 0x7FF) != 0);
	ctx.r7.s64 = ctx.r7.s32 >> 11;
	// mulli r4,r4,2896
	ctx.r4.s64 = static_cast<int64_t>(ctx.r4.u64 * static_cast<uint64_t>(2896));
	// srawi r28,r28,11
	ctx.xer.ca = (ctx.r28.s32 < 0) & ((ctx.r28.u32 & 0x7FF) != 0);
	ctx.r28.s64 = ctx.r28.s32 >> 11;
	// mulli r30,r26,2217
	ctx.r30.s64 = static_cast<int64_t>(ctx.r26.u64 * static_cast<uint64_t>(2217));
	// srawi r26,r4,11
	ctx.xer.ca = (ctx.r4.s32 < 0) & ((ctx.r4.u32 & 0x7FF) != 0);
	ctx.r26.s64 = ctx.r4.s32 >> 11;
	// subf r4,r8,r28
	ctx.r4.u64 = ctx.r28.u64 - ctx.r8.u64;
	// add r28,r8,r27
	ctx.r28.u64 = ctx.r8.u64 + ctx.r27.u64;
	// subf r8,r8,r27
	ctx.r8.u64 = ctx.r27.u64 - ctx.r8.u64;
	// srawi r30,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r30.s64 = ctx.r30.s32 >> 11;
	// addi r6,r6,4
	ctx.r6.s64 = ctx.r6.s64 + 4;
	// stw r28,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r28.u32);
	// stw r8,224(r11)
	REX_STORE_U32(ctx.r11.u32 + 224, ctx.r8.u32);
	// add r8,r4,r7
	ctx.r8.u64 = ctx.r4.u64 + ctx.r7.u64;
	// subf r4,r7,r30
	ctx.r4.u64 = ctx.r30.u64 - ctx.r7.u64;
	// subf r7,r8,r26
	ctx.r7.u64 = ctx.r26.u64 - ctx.r8.u64;
	// add r30,r8,r29
	ctx.r30.u64 = ctx.r8.u64 + ctx.r29.u64;
	// subf r8,r8,r29
	ctx.r8.u64 = ctx.r29.u64 - ctx.r8.u64;
	// stw r30,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r30.u32);
	// stw r8,192(r11)
	REX_STORE_U32(ctx.r11.u32 + 192, ctx.r8.u32);
	// add r8,r4,r7
	ctx.r8.u64 = ctx.r4.u64 + ctx.r7.u64;
	// add r4,r7,r5
	ctx.r4.u64 = ctx.r7.u64 + ctx.r5.u64;
	// subf r7,r7,r5
	ctx.r7.u64 = ctx.r5.u64 - ctx.r7.u64;
	// stw r4,64(r11)
	REX_STORE_U32(ctx.r11.u32 + 64, ctx.r4.u32);
	// stw r7,160(r11)
	REX_STORE_U32(ctx.r11.u32 + 160, ctx.r7.u32);
	// add r7,r8,r31
	ctx.r7.u64 = ctx.r8.u64 + ctx.r31.u64;
	// subf r8,r8,r31
	ctx.r8.u64 = ctx.r31.u64 - ctx.r8.u64;
	// stw r7,128(r11)
	REX_STORE_U32(ctx.r11.u32 + 128, ctx.r7.u32);
loc_8295ECA4:
	// addi r24,r24,-1
	ctx.r24.s64 = ctx.r24.s64 + -1;
	// stw r8,96(r11)
	REX_STORE_U32(ctx.r11.u32 + 96, ctx.r8.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// cmpwi cr6,r24,0
	ctx.cr6.compare<int32_t>(ctx.r24.s32, 0, ctx.xer);
	// bgt cr6,0x8295eae4
	if (ctx.cr6.gt) goto loc_8295EAE4;
	// addi r11,r1,-344
	ctx.r11.s64 = ctx.r1.s64 + -344;
	// addi r10,r3,8
	ctx.r10.s64 = ctx.r3.s64 + 8;
	// li r6,8
	ctx.r6.s64 = 8;
loc_8295ECC4:
	// lwz r8,-4(r11)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r11.u32 + -4);
	// lwz r7,-12(r11)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r11.u32 + -12);
	// lwz r4,-20(r11)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r11.u32 + -20);
	// lwz r5,4(r11)
	ctx.r5.u64 = REX_LOAD_U32(ctx.r11.u32 + 4);
	// subf r28,r7,r8
	ctx.r28.u64 = ctx.r8.u64 - ctx.r7.u64;
	// lwz r3,0(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// add r27,r7,r8
	ctx.r27.u64 = ctx.r7.u64 + ctx.r8.u64;
	// lwz r31,-16(r11)
	ctx.r31.u64 = REX_LOAD_U32(ctx.r11.u32 + -16);
	// subf r26,r5,r4
	ctx.r26.u64 = ctx.r4.u64 - ctx.r5.u64;
	// lwz r30,-8(r11)
	ctx.r30.u64 = REX_LOAD_U32(ctx.r11.u32 + -8);
	// add r5,r4,r5
	ctx.r5.u64 = ctx.r4.u64 + ctx.r5.u64;
	// add r8,r31,r3
	ctx.r8.u64 = ctx.r31.u64 + ctx.r3.u64;
	// lwz r29,-24(r11)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r11.u32 + -24);
	// subf r7,r3,r31
	ctx.r7.u64 = ctx.r31.u64 - ctx.r3.u64;
	// add r31,r26,r28
	ctx.r31.u64 = ctx.r26.u64 + ctx.r28.u64;
	// mulli r7,r7,2896
	ctx.r7.s64 = static_cast<int64_t>(ctx.r7.u64 * static_cast<uint64_t>(2896));
	// subf r3,r30,r29
	ctx.r3.u64 = ctx.r29.u64 - ctx.r30.u64;
	// add r4,r29,r30
	ctx.r4.u64 = ctx.r29.u64 + ctx.r30.u64;
	// mulli r31,r31,3784
	ctx.r31.s64 = static_cast<int64_t>(ctx.r31.u64 * static_cast<uint64_t>(3784));
	// mulli r30,r28,-5352
	ctx.r30.s64 = static_cast<int64_t>(ctx.r28.u64 * static_cast<uint64_t>(-5352));
	// srawi r24,r7,11
	ctx.xer.ca = (ctx.r7.s32 < 0) & ((ctx.r7.u32 & 0x7FF) != 0);
	ctx.r24.s64 = ctx.r7.s32 >> 11;
	// srawi r31,r31,11
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0x7FF) != 0);
	ctx.r31.s64 = ctx.r31.s32 >> 11;
	// srawi r29,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r29.s64 = ctx.r30.s32 >> 11;
	// subf r30,r27,r5
	ctx.r30.u64 = ctx.r5.u64 - ctx.r27.u64;
	// add r7,r5,r27
	ctx.r7.u64 = ctx.r5.u64 + ctx.r27.u64;
	// mulli r30,r30,2896
	ctx.r30.s64 = static_cast<int64_t>(ctx.r30.u64 * static_cast<uint64_t>(2896));
	// srawi r28,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r28.s64 = ctx.r30.s32 >> 11;
	// mulli r30,r26,2217
	ctx.r30.s64 = static_cast<int64_t>(ctx.r26.u64 * static_cast<uint64_t>(2217));
	// srawi r27,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r27.s64 = ctx.r30.s32 >> 11;
	// add r30,r8,r4
	ctx.r30.u64 = ctx.r8.u64 + ctx.r4.u64;
	// subf r4,r8,r4
	ctx.r4.u64 = ctx.r4.u64 - ctx.r8.u64;
	// subf r5,r8,r24
	ctx.r5.u64 = ctx.r24.u64 - ctx.r8.u64;
	// subf r8,r7,r29
	ctx.r8.u64 = ctx.r29.u64 - ctx.r7.u64;
	// subf r29,r31,r27
	ctx.r29.u64 = ctx.r27.u64 - ctx.r31.u64;
	// add r8,r8,r31
	ctx.r8.u64 = ctx.r8.u64 + ctx.r31.u64;
	// add r31,r5,r3
	ctx.r31.u64 = ctx.r5.u64 + ctx.r3.u64;
	// subf r5,r5,r3
	ctx.r5.u64 = ctx.r3.u64 - ctx.r5.u64;
	// add r3,r7,r30
	ctx.r3.u64 = ctx.r7.u64 + ctx.r30.u64;
	// subf r7,r7,r30
	ctx.r7.u64 = ctx.r30.u64 - ctx.r7.u64;
	// addi r3,r3,127
	ctx.r3.s64 = ctx.r3.s64 + 127;
	// addi r30,r7,127
	ctx.r30.s64 = ctx.r7.s64 + 127;
	// srawi r7,r3,8
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0xFF) != 0);
	ctx.r7.s64 = ctx.r3.s32 >> 8;
	// add r3,r8,r31
	ctx.r3.u64 = ctx.r8.u64 + ctx.r31.u64;
	// rlwinm r27,r7,16,8,15
	ctx.r27.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 16) & 0xFF0000;
	// subf r7,r8,r28
	ctx.r7.u64 = ctx.r28.u64 - ctx.r8.u64;
	// subf r8,r8,r31
	ctx.r8.u64 = ctx.r31.u64 - ctx.r8.u64;
	// addi r3,r3,127
	ctx.r3.s64 = ctx.r3.s64 + 127;
	// addi r8,r8,127
	ctx.r8.s64 = ctx.r8.s64 + 127;
	// srawi r3,r3,8
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0xFF) != 0);
	ctx.r3.s64 = ctx.r3.s32 >> 8;
	// srawi r8,r8,8
	ctx.xer.ca = (ctx.r8.s32 < 0) & ((ctx.r8.u32 & 0xFF) != 0);
	ctx.r8.s64 = ctx.r8.s32 >> 8;
	// srawi r31,r30,8
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0xFF) != 0);
	ctx.r31.s64 = ctx.r30.s32 >> 8;
	// clrlwi r3,r3,24
	ctx.r3.u64 = ctx.r3.u32 & 0xFF;
	// rlwinm r30,r8,16,8,15
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 16) & 0xFF0000;
	// clrlwi r31,r31,24
	ctx.r31.u64 = ctx.r31.u32 & 0xFF;
	// or r8,r27,r3
	ctx.r8.u64 = ctx.r27.u64 | ctx.r3.u64;
	// or r3,r30,r31
	ctx.r3.u64 = ctx.r30.u64 | ctx.r31.u64;
	// add r31,r29,r7
	ctx.r31.u64 = ctx.r29.u64 + ctx.r7.u64;
	// subf r30,r7,r5
	ctx.r30.u64 = ctx.r5.u64 - ctx.r7.u64;
	// add r7,r7,r5
	ctx.r7.u64 = ctx.r7.u64 + ctx.r5.u64;
	// add r5,r31,r4
	ctx.r5.u64 = ctx.r31.u64 + ctx.r4.u64;
	// addi r29,r7,127
	ctx.r29.s64 = ctx.r7.s64 + 127;
	// addi r5,r5,127
	ctx.r5.s64 = ctx.r5.s64 + 127;
	// addi r30,r30,127
	ctx.r30.s64 = ctx.r30.s64 + 127;
	// subf r7,r31,r4
	ctx.r7.u64 = ctx.r4.u64 - ctx.r31.u64;
	// srawi r5,r5,8
	ctx.xer.ca = (ctx.r5.s32 < 0) & ((ctx.r5.u32 & 0xFF) != 0);
	ctx.r5.s64 = ctx.r5.s32 >> 8;
	// srawi r4,r30,8
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0xFF) != 0);
	ctx.r4.s64 = ctx.r30.s32 >> 8;
	// addi r7,r7,127
	ctx.r7.s64 = ctx.r7.s64 + 127;
	// clrlwi r4,r4,24
	ctx.r4.u64 = ctx.r4.u32 & 0xFF;
	// srawi r31,r29,8
	ctx.xer.ca = (ctx.r29.s32 < 0) & ((ctx.r29.u32 & 0xFF) != 0);
	ctx.r31.s64 = ctx.r29.s32 >> 8;
	// rlwinm r5,r5,16,8,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 16) & 0xFF0000;
	// srawi r30,r7,8
	ctx.xer.ca = (ctx.r7.s32 < 0) & ((ctx.r7.u32 & 0xFF) != 0);
	ctx.r30.s64 = ctx.r7.s32 >> 8;
	// or r7,r5,r4
	ctx.r7.u64 = ctx.r5.u64 | ctx.r4.u64;
	// clrlwi r4,r30,24
	ctx.r4.u64 = ctx.r30.u32 & 0xFF;
	// rlwinm r5,r31,16,8,15
	ctx.r5.u64 = __builtin_rotateleft64(ctx.r31.u32 | (ctx.r31.u64 << 32), 16) & 0xFF0000;
	// rlwinm r31,r8,8,0,23
	ctx.r31.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 8) & 0xFFFFFF00;
	// or r5,r5,r4
	ctx.r5.u64 = ctx.r5.u64 | ctx.r4.u64;
	// rlwinm r4,r3,8,0,23
	ctx.r4.u64 = __builtin_rotateleft64(ctx.r3.u32 | (ctx.r3.u64 << 32), 8) & 0xFFFFFF00;
	// rlwinm r30,r7,8,0,23
	ctx.r30.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 8) & 0xFFFFFF00;
	// or r8,r31,r8
	ctx.r8.u64 = ctx.r31.u64 | ctx.r8.u64;
	// or r4,r4,r3
	ctx.r4.u64 = ctx.r4.u64 | ctx.r3.u64;
	// or r7,r30,r7
	ctx.r7.u64 = ctx.r30.u64 | ctx.r7.u64;
	// rlwinm r3,r5,8,0,23
	ctx.r3.u64 = __builtin_rotateleft64(ctx.r5.u32 | (ctx.r5.u64 << 32), 8) & 0xFFFFFF00;
	// stw r8,-8(r10)
	REX_STORE_U32(ctx.r10.u32 + -8, ctx.r8.u32);
	// addi r6,r6,-1
	ctx.r6.s64 = ctx.r6.s64 + -1;
	// stw r4,4(r10)
	REX_STORE_U32(ctx.r10.u32 + 4, ctx.r4.u32);
	// or r5,r3,r5
	ctx.r5.u64 = ctx.r3.u64 | ctx.r5.u64;
	// stw r7,0(r10)
	REX_STORE_U32(ctx.r10.u32 + 0, ctx.r7.u32);
	// addi r11,r11,32
	ctx.r11.s64 = ctx.r11.s64 + 32;
	// cmplwi cr6,r6,0
	ctx.cr6.compare<uint32_t>(ctx.r6.u32, 0, ctx.xer);
	// stw r5,-4(r10)
	REX_STORE_U32(ctx.r10.u32 + -4, ctx.r5.u32);
	// add r10,r10,r25
	ctx.r10.u64 = ctx.r10.u64 + ctx.r25.u64;
	// stw r8,0(r9)
	REX_STORE_U32(ctx.r9.u32 + 0, ctx.r8.u32);
	// stw r5,4(r9)
	REX_STORE_U32(ctx.r9.u32 + 4, ctx.r5.u32);
	// stw r7,8(r9)
	REX_STORE_U32(ctx.r9.u32 + 8, ctx.r7.u32);
	// stw r4,12(r9)
	REX_STORE_U32(ctx.r9.u32 + 12, ctx.r4.u32);
	// add r9,r25,r9
	ctx.r9.u64 = ctx.r25.u64 + ctx.r9.u64;
	// bne cr6,0x8295ecc4
	if (!ctx.cr6.eq) goto loc_8295ECC4;
	// b 0x824131d4
	__restgprlr_19(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295EE50) {
	REX_FUNC_PROLOGUE(sub_8295EE50);
	// lis r11,-32228
	ctx.r11.s64 = -2112094208;
	// rlwinm r10,r6,8,0,23
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// addi r11,r11,25760
	ctx.r11.s64 = ctx.r11.s64 + 25760;
	// add r6,r10,r11
	ctx.r6.u64 = ctx.r10.u64 + ctx.r11.u64;
	// b 0x8295e7a0
	sub_8295E7A0(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295EE68) {
	REX_FUNC_PROLOGUE(sub_8295EE68);
	// lis r11,-32228
	ctx.r11.s64 = -2112094208;
	// rlwinm r10,r6,8,0,23
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// addi r11,r11,25760
	ctx.r11.s64 = ctx.r11.s64 + 25760;
	// add r6,r10,r11
	ctx.r6.u64 = ctx.r10.u64 + ctx.r11.u64;
	// b 0x8295eac8
	sub_8295EAC8(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295EE80) {
	REX_FUNC_PROLOGUE(sub_8295EE80);
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413180
	ctx.lr = 0x8295EE88;
	__savegprlr_18(ctx, base);
	// lis r11,-32228
	ctx.r11.s64 = -2112094208;
	// rlwinm r10,r6,8,0,23
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 8) & 0xFFFFFF00;
	// addi r11,r11,29864
	ctx.r11.s64 = ctx.r11.s64 + 29864;
	// mr r9,r5
	ctx.r9.u64 = ctx.r5.u64;
	// add r10,r10,r11
	ctx.r10.u64 = ctx.r10.u64 + ctx.r11.u64;
	// addi r11,r1,-384
	ctx.r11.s64 = ctx.r1.s64 + -384;
	// li r23,8
	ctx.r23.s64 = 8;
loc_8295EEA4:
	// lhz r31,48(r9)
	ctx.r31.u64 = REX_LOAD_U16(ctx.r9.u32 + 48);
	// lhz r5,16(r9)
	ctx.r5.u64 = REX_LOAD_U16(ctx.r9.u32 + 16);
	// lhz r6,32(r9)
	ctx.r6.u64 = REX_LOAD_U16(ctx.r9.u32 + 32);
	// extsh r28,r31
	ctx.r28.s64 = ctx.r31.s16;
	// lhz r31,80(r9)
	ctx.r31.u64 = REX_LOAD_U16(ctx.r9.u32 + 80);
	// extsh r29,r5
	ctx.r29.s64 = ctx.r5.s16;
	// lhz r30,96(r9)
	ctx.r30.u64 = REX_LOAD_U16(ctx.r9.u32 + 96);
	// extsh r6,r6
	ctx.r6.s64 = ctx.r6.s16;
	// extsh r27,r31
	ctx.r27.s64 = ctx.r31.s16;
	// lhz r5,64(r9)
	ctx.r5.u64 = REX_LOAD_U16(ctx.r9.u32 + 64);
	// extsh r31,r30
	ctx.r31.s64 = ctx.r30.s16;
	// lhz r26,112(r9)
	ctx.r26.u64 = REX_LOAD_U16(ctx.r9.u32 + 112);
	// or r30,r29,r6
	ctx.r30.u64 = ctx.r29.u64 | ctx.r6.u64;
	// extsh r5,r5
	ctx.r5.s64 = ctx.r5.s16;
	// or r30,r30,r28
	ctx.r30.u64 = ctx.r30.u64 | ctx.r28.u64;
	// extsh r26,r26
	ctx.r26.s64 = ctx.r26.s16;
	// or r30,r30,r5
	ctx.r30.u64 = ctx.r30.u64 | ctx.r5.u64;
	// or r30,r30,r27
	ctx.r30.u64 = ctx.r30.u64 | ctx.r27.u64;
	// or r30,r30,r31
	ctx.r30.u64 = ctx.r30.u64 | ctx.r31.u64;
	// or r30,r30,r26
	ctx.r30.u64 = ctx.r30.u64 | ctx.r26.u64;
	// extsh r30,r30
	ctx.r30.s64 = ctx.r30.s16;
	// cmpwi cr6,r30,0
	ctx.cr6.compare<int32_t>(ctx.r30.s32, 0, ctx.xer);
	// bne cr6,0x8295ef3c
	if (!ctx.cr6.eq) goto loc_8295EF3C;
	// lhz r6,0(r9)
	ctx.r6.u64 = REX_LOAD_U16(ctx.r9.u32 + 0);
	// addi r9,r9,2
	ctx.r9.s64 = ctx.r9.s64 + 2;
	// lwz r5,0(r10)
	ctx.r5.u64 = REX_LOAD_U32(ctx.r10.u32 + 0);
	// addi r10,r10,4
	ctx.r10.s64 = ctx.r10.s64 + 4;
	// extsh r6,r6
	ctx.r6.s64 = ctx.r6.s16;
	// mullw r6,r6,r5
	ctx.r6.s64 = int64_t(ctx.r6.s32) * int64_t(ctx.r5.s32);
	// srawi r6,r6,11
	ctx.xer.ca = (ctx.r6.s32 < 0) & ((ctx.r6.u32 & 0x7FF) != 0);
	ctx.r6.s64 = ctx.r6.s32 >> 11;
	// stw r6,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r6.u32);
	// stw r6,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r6.u32);
	// stw r6,64(r11)
	REX_STORE_U32(ctx.r11.u32 + 64, ctx.r6.u32);
	// stw r6,128(r11)
	REX_STORE_U32(ctx.r11.u32 + 128, ctx.r6.u32);
	// stw r6,160(r11)
	REX_STORE_U32(ctx.r11.u32 + 160, ctx.r6.u32);
	// stw r6,192(r11)
	REX_STORE_U32(ctx.r11.u32 + 192, ctx.r6.u32);
	// stw r6,224(r11)
	REX_STORE_U32(ctx.r11.u32 + 224, ctx.r6.u32);
	// b 0x8295f064
	goto loc_8295F064;
loc_8295EF3C:
	// lhz r30,0(r9)
	ctx.r30.u64 = REX_LOAD_U16(ctx.r9.u32 + 0);
	// addi r9,r9,2
	ctx.r9.s64 = ctx.r9.s64 + 2;
	// lwz r25,0(r10)
	ctx.r25.u64 = REX_LOAD_U32(ctx.r10.u32 + 0);
	// extsh r30,r30
	ctx.r30.s64 = ctx.r30.s16;
	// lwz r24,64(r10)
	ctx.r24.u64 = REX_LOAD_U32(ctx.r10.u32 + 64);
	// lwz r22,128(r10)
	ctx.r22.u64 = REX_LOAD_U32(ctx.r10.u32 + 128);
	// mullw r30,r30,r25
	ctx.r30.s64 = int64_t(ctx.r30.s32) * int64_t(ctx.r25.s32);
	// lwz r21,192(r10)
	ctx.r21.u64 = REX_LOAD_U32(ctx.r10.u32 + 192);
	// lwz r25,32(r10)
	ctx.r25.u64 = REX_LOAD_U32(ctx.r10.u32 + 32);
	// lwz r20,96(r10)
	ctx.r20.u64 = REX_LOAD_U32(ctx.r10.u32 + 96);
	// lwz r19,160(r10)
	ctx.r19.u64 = REX_LOAD_U32(ctx.r10.u32 + 160);
	// lwz r18,224(r10)
	ctx.r18.u64 = REX_LOAD_U32(ctx.r10.u32 + 224);
	// mullw r6,r24,r6
	ctx.r6.s64 = int64_t(ctx.r24.s32) * int64_t(ctx.r6.s32);
	// mullw r24,r22,r5
	ctx.r24.s64 = int64_t(ctx.r22.s32) * int64_t(ctx.r5.s32);
	// srawi r5,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r5.s64 = ctx.r30.s32 >> 11;
	// mullw r30,r21,r31
	ctx.r30.s64 = int64_t(ctx.r21.s32) * int64_t(ctx.r31.s32);
	// srawi r6,r6,11
	ctx.xer.ca = (ctx.r6.s32 < 0) & ((ctx.r6.u32 & 0x7FF) != 0);
	ctx.r6.s64 = ctx.r6.s32 >> 11;
	// srawi r31,r24,11
	ctx.xer.ca = (ctx.r24.s32 < 0) & ((ctx.r24.u32 & 0x7FF) != 0);
	ctx.r31.s64 = ctx.r24.s32 >> 11;
	// srawi r30,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r30.s64 = ctx.r30.s32 >> 11;
	// mullw r25,r25,r29
	ctx.r25.s64 = int64_t(ctx.r25.s32) * int64_t(ctx.r29.s32);
	// subf r24,r30,r6
	ctx.r24.u64 = ctx.r6.u64 - ctx.r30.u64;
	// add r6,r30,r6
	ctx.r6.u64 = ctx.r30.u64 + ctx.r6.u64;
	// mulli r30,r24,2896
	ctx.r30.s64 = static_cast<int64_t>(ctx.r24.u64 * static_cast<uint64_t>(2896));
	// mullw r28,r20,r28
	ctx.r28.s64 = int64_t(ctx.r20.s32) * int64_t(ctx.r28.s32);
	// srawi r24,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r24.s64 = ctx.r30.s32 >> 11;
	// mullw r27,r19,r27
	ctx.r27.s64 = int64_t(ctx.r19.s32) * int64_t(ctx.r27.s32);
	// add r29,r31,r5
	ctx.r29.u64 = ctx.r31.u64 + ctx.r5.u64;
	// mullw r26,r18,r26
	ctx.r26.s64 = int64_t(ctx.r18.s32) * int64_t(ctx.r26.s32);
	// srawi r30,r25,11
	ctx.xer.ca = (ctx.r25.s32 < 0) & ((ctx.r25.u32 & 0x7FF) != 0);
	ctx.r30.s64 = ctx.r25.s32 >> 11;
	// srawi r28,r28,11
	ctx.xer.ca = (ctx.r28.s32 < 0) & ((ctx.r28.u32 & 0x7FF) != 0);
	ctx.r28.s64 = ctx.r28.s32 >> 11;
	// srawi r27,r27,11
	ctx.xer.ca = (ctx.r27.s32 < 0) & ((ctx.r27.u32 & 0x7FF) != 0);
	ctx.r27.s64 = ctx.r27.s32 >> 11;
	// subf r5,r31,r5
	ctx.r5.u64 = ctx.r5.u64 - ctx.r31.u64;
	// add r25,r6,r29
	ctx.r25.u64 = ctx.r6.u64 + ctx.r29.u64;
	// srawi r26,r26,11
	ctx.xer.ca = (ctx.r26.s32 < 0) & ((ctx.r26.u32 & 0x7FF) != 0);
	ctx.r26.s64 = ctx.r26.s32 >> 11;
	// subf r31,r6,r24
	ctx.r31.u64 = ctx.r24.u64 - ctx.r6.u64;
	// subf r29,r6,r29
	ctx.r29.u64 = ctx.r29.u64 - ctx.r6.u64;
	// subf r6,r28,r27
	ctx.r6.u64 = ctx.r27.u64 - ctx.r28.u64;
	// subf r24,r26,r30
	ctx.r24.u64 = ctx.r30.u64 - ctx.r26.u64;
	// add r28,r27,r28
	ctx.r28.u64 = ctx.r27.u64 + ctx.r28.u64;
	// add r27,r31,r5
	ctx.r27.u64 = ctx.r31.u64 + ctx.r5.u64;
	// add r30,r26,r30
	ctx.r30.u64 = ctx.r26.u64 + ctx.r30.u64;
	// subf r31,r31,r5
	ctx.r31.u64 = ctx.r5.u64 - ctx.r31.u64;
	// add r5,r24,r6
	ctx.r5.u64 = ctx.r24.u64 + ctx.r6.u64;
	// mulli r26,r6,-5352
	ctx.r26.s64 = static_cast<int64_t>(ctx.r6.u64 * static_cast<uint64_t>(-5352));
	// add r6,r30,r28
	ctx.r6.u64 = ctx.r30.u64 + ctx.r28.u64;
	// subf r30,r28,r30
	ctx.r30.u64 = ctx.r30.u64 - ctx.r28.u64;
	// mulli r5,r5,3784
	ctx.r5.s64 = static_cast<int64_t>(ctx.r5.u64 * static_cast<uint64_t>(3784));
	// srawi r5,r5,11
	ctx.xer.ca = (ctx.r5.s32 < 0) & ((ctx.r5.u32 & 0x7FF) != 0);
	ctx.r5.s64 = ctx.r5.s32 >> 11;
	// mulli r30,r30,2896
	ctx.r30.s64 = static_cast<int64_t>(ctx.r30.u64 * static_cast<uint64_t>(2896));
	// srawi r26,r26,11
	ctx.xer.ca = (ctx.r26.s32 < 0) & ((ctx.r26.u32 & 0x7FF) != 0);
	ctx.r26.s64 = ctx.r26.s32 >> 11;
	// mulli r28,r24,2217
	ctx.r28.s64 = static_cast<int64_t>(ctx.r24.u64 * static_cast<uint64_t>(2217));
	// srawi r24,r30,11
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0x7FF) != 0);
	ctx.r24.s64 = ctx.r30.s32 >> 11;
	// subf r30,r6,r26
	ctx.r30.u64 = ctx.r26.u64 - ctx.r6.u64;
	// add r26,r6,r25
	ctx.r26.u64 = ctx.r6.u64 + ctx.r25.u64;
	// subf r6,r6,r25
	ctx.r6.u64 = ctx.r25.u64 - ctx.r6.u64;
	// srawi r28,r28,11
	ctx.xer.ca = (ctx.r28.s32 < 0) & ((ctx.r28.u32 & 0x7FF) != 0);
	ctx.r28.s64 = ctx.r28.s32 >> 11;
	// addi r10,r10,4
	ctx.r10.s64 = ctx.r10.s64 + 4;
	// stw r26,0(r11)
	REX_STORE_U32(ctx.r11.u32 + 0, ctx.r26.u32);
	// stw r6,224(r11)
	REX_STORE_U32(ctx.r11.u32 + 224, ctx.r6.u32);
	// add r6,r30,r5
	ctx.r6.u64 = ctx.r30.u64 + ctx.r5.u64;
	// subf r30,r5,r28
	ctx.r30.u64 = ctx.r28.u64 - ctx.r5.u64;
	// subf r5,r6,r24
	ctx.r5.u64 = ctx.r24.u64 - ctx.r6.u64;
	// add r28,r6,r27
	ctx.r28.u64 = ctx.r6.u64 + ctx.r27.u64;
	// subf r6,r6,r27
	ctx.r6.u64 = ctx.r27.u64 - ctx.r6.u64;
	// stw r28,32(r11)
	REX_STORE_U32(ctx.r11.u32 + 32, ctx.r28.u32);
	// stw r6,192(r11)
	REX_STORE_U32(ctx.r11.u32 + 192, ctx.r6.u32);
	// add r6,r30,r5
	ctx.r6.u64 = ctx.r30.u64 + ctx.r5.u64;
	// add r30,r5,r31
	ctx.r30.u64 = ctx.r5.u64 + ctx.r31.u64;
	// subf r5,r5,r31
	ctx.r5.u64 = ctx.r31.u64 - ctx.r5.u64;
	// stw r30,64(r11)
	REX_STORE_U32(ctx.r11.u32 + 64, ctx.r30.u32);
	// stw r5,160(r11)
	REX_STORE_U32(ctx.r11.u32 + 160, ctx.r5.u32);
	// add r5,r6,r29
	ctx.r5.u64 = ctx.r6.u64 + ctx.r29.u64;
	// subf r6,r6,r29
	ctx.r6.u64 = ctx.r29.u64 - ctx.r6.u64;
	// stw r5,128(r11)
	REX_STORE_U32(ctx.r11.u32 + 128, ctx.r5.u32);
loc_8295F064:
	// addi r23,r23,-1
	ctx.r23.s64 = ctx.r23.s64 + -1;
	// stw r6,96(r11)
	REX_STORE_U32(ctx.r11.u32 + 96, ctx.r6.u32);
	// addi r11,r11,4
	ctx.r11.s64 = ctx.r11.s64 + 4;
	// cmpwi cr6,r23,0
	ctx.cr6.compare<int32_t>(ctx.r23.s32, 0, ctx.xer);
	// bgt cr6,0x8295eea4
	if (ctx.cr6.gt) goto loc_8295EEA4;
	// addi r9,r3,1
	ctx.r9.s64 = ctx.r3.s64 + 1;
	// addi r10,r7,1
	ctx.r10.s64 = ctx.r7.s64 + 1;
	// addi r11,r1,-360
	ctx.r11.s64 = ctx.r1.s64 + -360;
	// li r5,8
	ctx.r5.s64 = 8;
loc_8295F088:
	// lwz r7,-4(r11)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r11.u32 + -4);
	// addi r5,r5,-1
	ctx.r5.s64 = ctx.r5.s64 + -1;
	// lwz r6,-12(r11)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r11.u32 + -12);
	// lwz r30,0(r11)
	ctx.r30.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);
	// lwz r29,-16(r11)
	ctx.r29.u64 = REX_LOAD_U32(ctx.r11.u32 + -16);
	// subf r26,r6,r7
	ctx.r26.u64 = ctx.r7.u64 - ctx.r6.u64;
	// add r25,r6,r7
	ctx.r25.u64 = ctx.r6.u64 + ctx.r7.u64;
	// lwz r31,-20(r11)
	ctx.r31.u64 = REX_LOAD_U32(ctx.r11.u32 + -20);
	// subf r6,r30,r29
	ctx.r6.u64 = ctx.r29.u64 - ctx.r30.u64;
	// lwz r3,4(r11)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 4);
	// add r7,r29,r30
	ctx.r7.u64 = ctx.r29.u64 + ctx.r30.u64;
	// lwz r28,-8(r11)
	ctx.r28.u64 = REX_LOAD_U32(ctx.r11.u32 + -8);
	// subf r24,r3,r31
	ctx.r24.u64 = ctx.r31.u64 - ctx.r3.u64;
	// lwz r27,-24(r11)
	ctx.r27.u64 = REX_LOAD_U32(ctx.r11.u32 + -24);
	// mulli r6,r6,2896
	ctx.r6.s64 = static_cast<int64_t>(ctx.r6.u64 * static_cast<uint64_t>(2896));
	// lbz r23,-1(r10)
	ctx.r23.u64 = REX_LOAD_U8(ctx.r10.u32 + -1);
	// add r3,r31,r3
	ctx.r3.u64 = ctx.r31.u64 + ctx.r3.u64;
	// srawi r22,r6,11
	ctx.xer.ca = (ctx.r6.s32 < 0) & ((ctx.r6.u32 & 0x7FF) != 0);
	ctx.r22.s64 = ctx.r6.s32 >> 11;
	// add r29,r24,r26
	ctx.r29.u64 = ctx.r24.u64 + ctx.r26.u64;
	// add r6,r3,r25
	ctx.r6.u64 = ctx.r3.u64 + ctx.r25.u64;
	// subf r3,r25,r3
	ctx.r3.u64 = ctx.r3.u64 - ctx.r25.u64;
	// mulli r29,r29,3784
	ctx.r29.s64 = static_cast<int64_t>(ctx.r29.u64 * static_cast<uint64_t>(3784));
	// mulli r21,r26,-5352
	ctx.r21.s64 = static_cast<int64_t>(ctx.r26.u64 * static_cast<uint64_t>(-5352));
	// add r31,r27,r28
	ctx.r31.u64 = ctx.r27.u64 + ctx.r28.u64;
	// mulli r3,r3,2896
	ctx.r3.s64 = static_cast<int64_t>(ctx.r3.u64 * static_cast<uint64_t>(2896));
	// srawi r29,r29,11
	ctx.xer.ca = (ctx.r29.s32 < 0) & ((ctx.r29.u32 & 0x7FF) != 0);
	ctx.r29.s64 = ctx.r29.s32 >> 11;
	// subf r30,r28,r27
	ctx.r30.u64 = ctx.r27.u64 - ctx.r28.u64;
	// srawi r26,r21,11
	ctx.xer.ca = (ctx.r21.s32 < 0) & ((ctx.r21.u32 & 0x7FF) != 0);
	ctx.r26.s64 = ctx.r21.s32 >> 11;
	// mulli r25,r24,2217
	ctx.r25.s64 = static_cast<int64_t>(ctx.r24.u64 * static_cast<uint64_t>(2217));
	// add r27,r7,r31
	ctx.r27.u64 = ctx.r7.u64 + ctx.r31.u64;
	// srawi r24,r3,11
	ctx.xer.ca = (ctx.r3.s32 < 0) & ((ctx.r3.u32 & 0x7FF) != 0);
	ctx.r24.s64 = ctx.r3.s32 >> 11;
	// subf r3,r7,r31
	ctx.r3.u64 = ctx.r31.u64 - ctx.r7.u64;
	// subf r28,r7,r22
	ctx.r28.u64 = ctx.r22.u64 - ctx.r7.u64;
	// subf r7,r6,r26
	ctx.r7.u64 = ctx.r26.u64 - ctx.r6.u64;
	// add r26,r6,r27
	ctx.r26.u64 = ctx.r6.u64 + ctx.r27.u64;
	// add r31,r28,r30
	ctx.r31.u64 = ctx.r28.u64 + ctx.r30.u64;
	// addi r26,r26,127
	ctx.r26.s64 = ctx.r26.s64 + 127;
	// subf r30,r28,r30
	ctx.r30.u64 = ctx.r30.u64 - ctx.r28.u64;
	// subf r28,r6,r27
	ctx.r28.u64 = ctx.r27.u64 - ctx.r6.u64;
	// srawi r25,r25,11
	ctx.xer.ca = (ctx.r25.s32 < 0) & ((ctx.r25.u32 & 0x7FF) != 0);
	ctx.r25.s64 = ctx.r25.s32 >> 11;
	// srawi r6,r26,8
	ctx.xer.ca = (ctx.r26.s32 < 0) & ((ctx.r26.u32 & 0xFF) != 0);
	ctx.r6.s64 = ctx.r26.s32 >> 8;
	// addi r28,r28,127
	ctx.r28.s64 = ctx.r28.s64 + 127;
	// add r27,r6,r23
	ctx.r27.u64 = ctx.r6.u64 + ctx.r23.u64;
	// srawi r6,r28,8
	ctx.xer.ca = (ctx.r28.s32 < 0) & ((ctx.r28.u32 & 0xFF) != 0);
	ctx.r6.s64 = ctx.r28.s32 >> 8;
	// add r7,r7,r29
	ctx.r7.u64 = ctx.r7.u64 + ctx.r29.u64;
	// subf r29,r29,r25
	ctx.r29.u64 = ctx.r25.u64 - ctx.r29.u64;
	// addi r11,r11,32
	ctx.r11.s64 = ctx.r11.s64 + 32;
	// stb r27,-1(r9)
	REX_STORE_U8(ctx.r9.u32 + -1, ctx.r27.u8);
	// add r27,r7,r31
	ctx.r27.u64 = ctx.r7.u64 + ctx.r31.u64;
	// lbz r28,6(r10)
	ctx.r28.u64 = REX_LOAD_U8(ctx.r10.u32 + 6);
	// add r6,r6,r28
	ctx.r6.u64 = ctx.r6.u64 + ctx.r28.u64;
	// stb r6,6(r9)
	REX_STORE_U8(ctx.r9.u32 + 6, ctx.r6.u8);
	// subf r6,r7,r24
	ctx.r6.u64 = ctx.r24.u64 - ctx.r7.u64;
	// subf r7,r7,r31
	ctx.r7.u64 = ctx.r31.u64 - ctx.r7.u64;
	// lbz r28,0(r10)
	ctx.r28.u64 = REX_LOAD_U8(ctx.r10.u32 + 0);
	// addi r31,r27,127
	ctx.r31.s64 = ctx.r27.s64 + 127;
	// addi r7,r7,127
	ctx.r7.s64 = ctx.r7.s64 + 127;
	// srawi r31,r31,8
	ctx.xer.ca = (ctx.r31.s32 < 0) & ((ctx.r31.u32 & 0xFF) != 0);
	ctx.r31.s64 = ctx.r31.s32 >> 8;
	// srawi r7,r7,8
	ctx.xer.ca = (ctx.r7.s32 < 0) & ((ctx.r7.u32 & 0xFF) != 0);
	ctx.r7.s64 = ctx.r7.s32 >> 8;
	// add r31,r31,r28
	ctx.r31.u64 = ctx.r31.u64 + ctx.r28.u64;
	// stb r31,0(r9)
	REX_STORE_U8(ctx.r9.u32 + 0, ctx.r31.u8);
	// lbz r31,5(r10)
	ctx.r31.u64 = REX_LOAD_U8(ctx.r10.u32 + 5);
	// add r7,r7,r31
	ctx.r7.u64 = ctx.r7.u64 + ctx.r31.u64;
	// stb r7,5(r9)
	REX_STORE_U8(ctx.r9.u32 + 5, ctx.r7.u8);
	// add r7,r6,r30
	ctx.r7.u64 = ctx.r6.u64 + ctx.r30.u64;
	// subf r30,r6,r30
	ctx.r30.u64 = ctx.r30.u64 - ctx.r6.u64;
	// lbz r31,1(r10)
	ctx.r31.u64 = REX_LOAD_U8(ctx.r10.u32 + 1);
	// addi r28,r7,127
	ctx.r28.s64 = ctx.r7.s64 + 127;
	// add r7,r29,r6
	ctx.r7.u64 = ctx.r29.u64 + ctx.r6.u64;
	// srawi r6,r28,8
	ctx.xer.ca = (ctx.r28.s32 < 0) & ((ctx.r28.u32 & 0xFF) != 0);
	ctx.r6.s64 = ctx.r28.s32 >> 8;
	// addi r30,r30,127
	ctx.r30.s64 = ctx.r30.s64 + 127;
	// add r6,r6,r31
	ctx.r6.u64 = ctx.r6.u64 + ctx.r31.u64;
	// srawi r31,r30,8
	ctx.xer.ca = (ctx.r30.s32 < 0) & ((ctx.r30.u32 & 0xFF) != 0);
	ctx.r31.s64 = ctx.r30.s32 >> 8;
	// stb r6,1(r9)
	REX_STORE_U8(ctx.r9.u32 + 1, ctx.r6.u8);
	// lbz r6,4(r10)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r10.u32 + 4);
	// mr r30,r6
	ctx.r30.u64 = ctx.r6.u64;
	// subf r6,r7,r3
	ctx.r6.u64 = ctx.r3.u64 - ctx.r7.u64;
	// add r31,r31,r30
	ctx.r31.u64 = ctx.r31.u64 + ctx.r30.u64;
	// addi r6,r6,127
	ctx.r6.s64 = ctx.r6.s64 + 127;
	// add r7,r7,r3
	ctx.r7.u64 = ctx.r7.u64 + ctx.r3.u64;
	// srawi r6,r6,8
	ctx.xer.ca = (ctx.r6.s32 < 0) & ((ctx.r6.u32 & 0xFF) != 0);
	ctx.r6.s64 = ctx.r6.s32 >> 8;
	// stb r31,4(r9)
	REX_STORE_U8(ctx.r9.u32 + 4, ctx.r31.u8);
	// addi r7,r7,127
	ctx.r7.s64 = ctx.r7.s64 + 127;
	// lbz r3,2(r10)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r10.u32 + 2);
	// cmplwi cr6,r5,0
	ctx.cr6.compare<uint32_t>(ctx.r5.u32, 0, ctx.xer);
	// srawi r7,r7,8
	ctx.xer.ca = (ctx.r7.s32 < 0) & ((ctx.r7.u32 & 0xFF) != 0);
	ctx.r7.s64 = ctx.r7.s32 >> 8;
	// add r6,r6,r3
	ctx.r6.u64 = ctx.r6.u64 + ctx.r3.u64;
	// stb r6,2(r9)
	REX_STORE_U8(ctx.r9.u32 + 2, ctx.r6.u8);
	// lbz r6,3(r10)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r10.u32 + 3);
	// add r10,r10,r8
	ctx.r10.u64 = ctx.r10.u64 + ctx.r8.u64;
	// add r7,r7,r6
	ctx.r7.u64 = ctx.r7.u64 + ctx.r6.u64;
	// stb r7,3(r9)
	REX_STORE_U8(ctx.r9.u32 + 3, ctx.r7.u8);
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// bne cr6,0x8295f088
	if (!ctx.cr6.eq) goto loc_8295F088;
	// b 0x824131d0
	__restgprlr_18(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295F208) {
	REX_FUNC_PROLOGUE(sub_8295F208);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413180
	ctx.lr = 0x8295F210;
	__savegprlr_18(ctx, base);
	// stwu r1,-464(r1)
	ea = -464 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r22,0
	ctx.r22.s64 = 0;
	// mr r24,r3
	ctx.r24.u64 = ctx.r3.u64;
	// mr r21,r4
	ctx.r21.u64 = ctx.r4.u64;
	// li r5,120
	ctx.r5.s64 = 120;
	// li r4,0
	ctx.r4.s64 = 0;
	// addi r3,r1,88
	ctx.r3.s64 = ctx.r1.s64 + 88;
	// std r22,80(r1)
	REX_STORE_U64(ctx.r1.u32 + 80, ctx.r22.u64);
	// bl 0x82414530
	ctx.lr = 0x8295F234;
	sub_82414530(ctx, base);
	// lwz r11,8(r21)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r21.u32 + 8);
	// lwz r10,0(r21)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r21.u32 + 0);
	// lwz r5,4(r21)
	ctx.r5.u64 = REX_LOAD_U32(ctx.r21.u32 + 4);
	// cmplwi cr6,r11,4
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 4, ctx.xer);
	// bge cr6,0x8295f274
	if (!ctx.cr6.lt) goto loc_8295F274;
	// lwz r9,0(r5)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// clrlwi r10,r10,24
	ctx.r10.u64 = ctx.r10.u32 & 0xFF;
	// subfic r8,r11,4
	ctx.xer.ca = ctx.r11.u32 <= 4;
	ctx.r8.u64 = static_cast<uint64_t>(4) - ctx.r11.u64;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// slw r7,r9,r11
	ctx.r7.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r11.u8 & 0x3F));
	// addi r11,r11,28
	ctx.r11.s64 = ctx.r11.s64 + 28;
	// clrlwi r7,r7,24
	ctx.r7.u64 = ctx.r7.u32 & 0xFF;
	// or r10,r7,r10
	ctx.r10.u64 = ctx.r7.u64 | ctx.r10.u64;
	// clrlwi r27,r10,28
	ctx.r27.u64 = ctx.r10.u32 & 0xF;
	// srw r10,r9,r8
	ctx.r10.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r8.u8 & 0x3F));
	// b 0x8295f284
	goto loc_8295F284;
loc_8295F274:
	// mr r9,r10
	ctx.r9.u64 = ctx.r10.u64;
	// rlwinm r10,r10,28,4,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 28) & 0xFFFFFFF;
	// clrlwi r27,r9,28
	ctx.r27.u64 = ctx.r9.u32 & 0xF;
	// addi r11,r11,-4
	ctx.r11.s64 = ctx.r11.s64 + -4;
loc_8295F284:
	// li r8,16
	ctx.r8.s64 = 16;
	// addi r9,r27,-1
	ctx.r9.s64 = ctx.r27.s64 + -1;
	// li r23,1
	ctx.r23.s64 = 1;
	// addi r26,r1,276
	ctx.r26.s64 = ctx.r1.s64 + 276;
	// addi r25,r1,282
	ctx.r25.s64 = ctx.r1.s64 + 282;
	// stb r8,276(r1)
	REX_STORE_U8(ctx.r1.u32 + 276, ctx.r8.u8);
	// li r8,96
	ctx.r8.s64 = 96;
	// mr r7,r27
	ctx.r7.u64 = ctx.r27.u64;
	// cmplwi cr6,r27,1
	ctx.cr6.compare<uint32_t>(ctx.r27.u32, 1, ctx.xer);
	// stb r8,277(r1)
	REX_STORE_U8(ctx.r1.u32 + 277, ctx.r8.u8);
	// li r8,176
	ctx.r8.s64 = 176;
	// stb r8,278(r1)
	REX_STORE_U8(ctx.r1.u32 + 278, ctx.r8.u8);
	// li r8,7
	ctx.r8.s64 = 7;
	// slw r29,r23,r9
	ctx.r29.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r23.u32 << (ctx.r9.u8 & 0x3F));
	// stb r8,279(r1)
	REX_STORE_U8(ctx.r1.u32 + 279, ctx.r8.u8);
	// li r8,11
	ctx.r8.s64 = 11;
	// stb r8,280(r1)
	REX_STORE_U8(ctx.r1.u32 + 280, ctx.r8.u8);
	// li r8,15
	ctx.r8.s64 = 15;
	// stb r8,281(r1)
	REX_STORE_U8(ctx.r1.u32 + 281, ctx.r8.u8);
	// ble cr6,0x8295f724
	if (!ctx.cr6.gt) goto loc_8295F724;
	// li r28,-1
	ctx.r28.s64 = -1;
loc_8295F2D8:
	// addi r4,r7,1
	ctx.r4.s64 = ctx.r7.s64 + 1;
	// mr r3,r26
	ctx.r3.u64 = ctx.r26.u64;
	// addi r9,r4,-2
	ctx.r9.s64 = ctx.r4.s64 + -2;
	// cmplw cr6,r26,r25
	ctx.cr6.compare<uint32_t>(ctx.r26.u32, ctx.r25.u32, ctx.xer);
	// slw r31,r23,r9
	ctx.r31.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r23.u32 << (ctx.r9.u8 & 0x3F));
	// addi r30,r31,-1
	ctx.r30.s64 = ctx.r31.s64 + -1;
	// bge cr6,0x8295f714
	if (!ctx.cr6.lt) goto loc_8295F714;
loc_8295F2F4:
	// lbz r8,0(r3)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r3.u32 + 0);
	// cmplwi cr6,r8,0
	ctx.cr6.compare<uint32_t>(ctx.r8.u32, 0, ctx.xer);
	// beq cr6,0x8295f708
	if (ctx.cr6.eq) goto loc_8295F708;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295f31c
	if (!ctx.cr6.eq) goto loc_8295F31C;
	// lwz r9,0(r5)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// rlwinm r10,r9,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295f328
	goto loc_8295F328;
loc_8295F31C:
	// mr r9,r10
	ctx.r9.u64 = ctx.r10.u64;
	// rlwinm r10,r10,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295F328:
	// clrlwi r9,r9,31
	ctx.r9.u64 = ctx.r9.u32 & 0x1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// beq cr6,0x8295f708
	if (ctx.cr6.eq) goto loc_8295F708;
	// clrlwi r9,r8,30
	ctx.r9.u64 = ctx.r8.u32 & 0x3;
	// cmplwi cr6,r9,3
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 3, ctx.xer);
	// bgt cr6,0x8295f708
	if (ctx.cr6.gt) goto loc_8295F708;
	// lis r12,-32106
	ctx.r12.s64 = -2104098816;
	// addi r12,r12,-3240
	ctx.r12.s64 = ctx.r12.s64 + -3240;
	// rlwinm r0,r9,2,0,29
	ctx.r0.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// lwzx r0,r12,r0
	ctx.r0.u64 = REX_LOAD_U32(ctx.r12.u32 + ctx.r0.u32);
	// mtctr r0
	ctx.ctr.u64 = ctx.r0.u64;
	// bctr 
	switch (ctx.r9.u32) {
	case 0:
		goto loc_8295F368;
	case 1:
		goto loc_8295F37C;
	case 2:
		goto loc_8295F3B4;
	case 3:
		goto loc_8295F690;
	default:
		__builtin_trap(); // Switch case out of range
	}
loc_8295F368:
	// rlwinm r6,r8,30,2,31
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 30) & 0x3FFFFFFF;
	// rlwinm r9,r6,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r9,r9,17
	ctx.r9.s64 = ctx.r9.s64 + 17;
	// stb r9,0(r3)
	REX_STORE_U8(ctx.r3.u32 + 0, ctx.r9.u8);
	// b 0x8295f3c0
	goto loc_8295F3C0;
loc_8295F37C:
	// rlwinm r9,r8,0,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 0) & 0xFFFFFFFC;
	// addi r8,r9,2
	ctx.r8.s64 = ctx.r9.s64 + 2;
	// addi r6,r9,18
	ctx.r6.s64 = ctx.r9.s64 + 18;
	// addi r20,r9,34
	ctx.r20.s64 = ctx.r9.s64 + 34;
	// addi r9,r9,50
	ctx.r9.s64 = ctx.r9.s64 + 50;
	// stb r8,0(r3)
	REX_STORE_U8(ctx.r3.u32 + 0, ctx.r8.u8);
	// mr r8,r9
	ctx.r8.u64 = ctx.r9.u64;
	// addi r9,r25,1
	ctx.r9.s64 = ctx.r25.s64 + 1;
	// stb r6,0(r25)
	REX_STORE_U8(ctx.r25.u32 + 0, ctx.r6.u8);
	// stb r20,0(r9)
	REX_STORE_U8(ctx.r9.u32 + 0, ctx.r20.u8);
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// addi r25,r9,1
	ctx.r25.s64 = ctx.r9.s64 + 1;
	// stb r8,0(r9)
	REX_STORE_U8(ctx.r9.u32 + 0, ctx.r8.u8);
	// b 0x8295f70c
	goto loc_8295F70C;
loc_8295F3B4:
	// stb r22,0(r3)
	REX_STORE_U8(ctx.r3.u32 + 0, ctx.r22.u8);
	// rlwinm r6,r8,30,2,31
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 30) & 0x3FFFFFFF;
	// addi r3,r3,1
	ctx.r3.s64 = ctx.r3.s64 + 1;
loc_8295F3C0:
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295f51c
	if (!ctx.cr6.eq) goto loc_8295F51C;
	// lwz r10,0(r5)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// clrlwi r11,r10,31
	ctx.r11.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295f500
	if (!ctx.cr6.eq) goto loc_8295F500;
	// rlwinm r9,r10,31,1,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// subfic r11,r7,31
	ctx.xer.ca = ctx.r7.u32 <= 31;
	ctx.r11.u64 = static_cast<uint64_t>(31) - ctx.r7.u64;
loc_8295F3E4:
	// srw r10,r10,r4
	ctx.r10.u64 = ctx.r4.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r4.u8 & 0x3F));
loc_8295F3E8:
	// and r20,r9,r31
	ctx.r20.u64 = ctx.r9.u64 & ctx.r31.u64;
	// and r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 & ctx.r30.u64;
	// cmplwi cr6,r20,0
	ctx.cr6.compare<uint32_t>(ctx.r20.u32, 0, ctx.xer);
	// or r9,r8,r29
	ctx.r9.u64 = ctx.r8.u64 | ctx.r29.u64;
	// beq cr6,0x8295f400
	if (ctx.cr6.eq) goto loc_8295F400;
	// neg r9,r9
	ctx.r9.s64 = static_cast<int64_t>(-ctx.r9.u64);
loc_8295F400:
	// rlwinm r8,r6,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 1) & 0xFFFFFFFE;
	// addi r20,r1,80
	ctx.r20.s64 = ctx.r1.s64 + 80;
	// sthx r9,r8,r20
	REX_STORE_U16(ctx.r8.u32 + ctx.r20.u32, ctx.r9.u16);
loc_8295F40C:
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295f580
	if (!ctx.cr6.eq) goto loc_8295F580;
	// lwz r10,0(r5)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// clrlwi r11,r10,31
	ctx.r11.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295f564
	if (!ctx.cr6.eq) goto loc_8295F564;
	// rlwinm r9,r10,31,1,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// subfic r11,r7,31
	ctx.xer.ca = ctx.r7.u32 <= 31;
	ctx.r11.u64 = static_cast<uint64_t>(31) - ctx.r7.u64;
loc_8295F434:
	// srw r10,r10,r4
	ctx.r10.u64 = ctx.r4.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r4.u8 & 0x3F));
loc_8295F438:
	// and r20,r9,r31
	ctx.r20.u64 = ctx.r9.u64 & ctx.r31.u64;
	// and r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 & ctx.r30.u64;
	// cmplwi cr6,r20,0
	ctx.cr6.compare<uint32_t>(ctx.r20.u32, 0, ctx.xer);
	// or r9,r8,r29
	ctx.r9.u64 = ctx.r8.u64 | ctx.r29.u64;
	// beq cr6,0x8295f450
	if (ctx.cr6.eq) goto loc_8295F450;
	// neg r9,r9
	ctx.r9.s64 = static_cast<int64_t>(-ctx.r9.u64);
loc_8295F450:
	// rlwinm r8,r6,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 1) & 0xFFFFFFFE;
	// addi r20,r1,80
	ctx.r20.s64 = ctx.r1.s64 + 80;
	// sthx r9,r8,r20
	REX_STORE_U16(ctx.r8.u32 + ctx.r20.u32, ctx.r9.u16);
loc_8295F45C:
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295f5e4
	if (!ctx.cr6.eq) goto loc_8295F5E4;
	// lwz r10,0(r5)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// clrlwi r11,r10,31
	ctx.r11.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295f5c8
	if (!ctx.cr6.eq) goto loc_8295F5C8;
	// rlwinm r9,r10,31,1,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// subfic r11,r7,31
	ctx.xer.ca = ctx.r7.u32 <= 31;
	ctx.r11.u64 = static_cast<uint64_t>(31) - ctx.r7.u64;
loc_8295F484:
	// srw r10,r10,r4
	ctx.r10.u64 = ctx.r4.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r4.u8 & 0x3F));
loc_8295F488:
	// and r20,r9,r31
	ctx.r20.u64 = ctx.r9.u64 & ctx.r31.u64;
	// and r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 & ctx.r30.u64;
	// cmplwi cr6,r20,0
	ctx.cr6.compare<uint32_t>(ctx.r20.u32, 0, ctx.xer);
	// or r9,r8,r29
	ctx.r9.u64 = ctx.r8.u64 | ctx.r29.u64;
	// beq cr6,0x8295f4a0
	if (ctx.cr6.eq) goto loc_8295F4A0;
	// neg r9,r9
	ctx.r9.s64 = static_cast<int64_t>(-ctx.r9.u64);
loc_8295F4A0:
	// rlwinm r8,r6,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 1) & 0xFFFFFFFE;
	// addi r20,r1,80
	ctx.r20.s64 = ctx.r1.s64 + 80;
	// sthx r9,r8,r20
	REX_STORE_U16(ctx.r8.u32 + ctx.r20.u32, ctx.r9.u16);
loc_8295F4AC:
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295f648
	if (!ctx.cr6.eq) goto loc_8295F648;
	// lwz r10,0(r5)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// clrlwi r11,r10,31
	ctx.r11.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295f62c
	if (!ctx.cr6.eq) goto loc_8295F62C;
	// rlwinm r9,r10,31,1,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// subfic r11,r7,31
	ctx.xer.ca = ctx.r7.u32 <= 31;
	ctx.r11.u64 = static_cast<uint64_t>(31) - ctx.r7.u64;
loc_8295F4D4:
	// srw r10,r10,r4
	ctx.r10.u64 = ctx.r4.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r4.u8 & 0x3F));
loc_8295F4D8:
	// and r20,r9,r31
	ctx.r20.u64 = ctx.r9.u64 & ctx.r31.u64;
	// and r8,r9,r30
	ctx.r8.u64 = ctx.r9.u64 & ctx.r30.u64;
	// cmplwi cr6,r20,0
	ctx.cr6.compare<uint32_t>(ctx.r20.u32, 0, ctx.xer);
	// or r9,r8,r29
	ctx.r9.u64 = ctx.r8.u64 | ctx.r29.u64;
	// beq cr6,0x8295f4f0
	if (ctx.cr6.eq) goto loc_8295F4F0;
	// neg r9,r9
	ctx.r9.s64 = static_cast<int64_t>(-ctx.r9.u64);
loc_8295F4F0:
	// rlwinm r8,r6,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 1) & 0xFFFFFFFE;
	// addi r6,r1,80
	ctx.r6.s64 = ctx.r1.s64 + 80;
	// sthx r9,r8,r6
	REX_STORE_U16(ctx.r8.u32 + ctx.r6.u32, ctx.r9.u16);
	// b 0x8295f70c
	goto loc_8295F70C;
loc_8295F500:
	// li r11,31
	ctx.r11.s64 = 31;
loc_8295F504:
	// rlwinm r9,r6,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r26,r26,-1
	ctx.r26.s64 = ctx.r26.s64 + -1;
	// addi r9,r9,3
	ctx.r9.s64 = ctx.r9.s64 + 3;
	// rlwinm r10,r10,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// stb r9,0(r26)
	REX_STORE_U8(ctx.r26.u32 + 0, ctx.r9.u8);
	// b 0x8295f40c
	goto loc_8295F40C;
loc_8295F51C:
	// clrlwi r9,r10,31
	ctx.r9.u64 = ctx.r10.u32 & 0x1;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// bne cr6,0x8295f504
	if (!ctx.cr6.eq) goto loc_8295F504;
	// cmplw cr6,r11,r7
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r7.u32, ctx.xer);
	// rlwinm r9,r10,31,1,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// bge cr6,0x8295f55c
	if (!ctx.cr6.lt) goto loc_8295F55C;
	// lwz r8,0(r5)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// subf r20,r11,r7
	ctx.r20.u64 = ctx.r7.u64 - ctx.r11.u64;
	// subf r10,r7,r11
	ctx.r10.u64 = ctx.r11.u64 - ctx.r7.u64;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// slw r19,r8,r11
	ctx.r19.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 << (ctx.r11.u8 & 0x3F));
	// addi r11,r10,32
	ctx.r11.s64 = ctx.r10.s64 + 32;
	// or r9,r19,r9
	ctx.r9.u64 = ctx.r19.u64 | ctx.r9.u64;
	// srw r10,r8,r20
	ctx.r10.u64 = ctx.r20.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r20.u8 & 0x3F));
	// b 0x8295f3e8
	goto loc_8295F3E8;
loc_8295F55C:
	// subf r11,r7,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r7.u64;
	// b 0x8295f3e4
	goto loc_8295F3E4;
loc_8295F564:
	// li r11,31
	ctx.r11.s64 = 31;
loc_8295F568:
	// rlwinm r9,r6,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r26,r26,-1
	ctx.r26.s64 = ctx.r26.s64 + -1;
	// addi r9,r9,3
	ctx.r9.s64 = ctx.r9.s64 + 3;
	// rlwinm r10,r10,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// stb r9,0(r26)
	REX_STORE_U8(ctx.r26.u32 + 0, ctx.r9.u8);
	// b 0x8295f45c
	goto loc_8295F45C;
loc_8295F580:
	// clrlwi r9,r10,31
	ctx.r9.u64 = ctx.r10.u32 & 0x1;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// bne cr6,0x8295f568
	if (!ctx.cr6.eq) goto loc_8295F568;
	// cmplw cr6,r11,r7
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r7.u32, ctx.xer);
	// rlwinm r9,r10,31,1,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// bge cr6,0x8295f5c0
	if (!ctx.cr6.lt) goto loc_8295F5C0;
	// lwz r8,0(r5)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// subf r20,r11,r7
	ctx.r20.u64 = ctx.r7.u64 - ctx.r11.u64;
	// subf r10,r7,r11
	ctx.r10.u64 = ctx.r11.u64 - ctx.r7.u64;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// slw r19,r8,r11
	ctx.r19.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 << (ctx.r11.u8 & 0x3F));
	// addi r11,r10,32
	ctx.r11.s64 = ctx.r10.s64 + 32;
	// or r9,r19,r9
	ctx.r9.u64 = ctx.r19.u64 | ctx.r9.u64;
	// srw r10,r8,r20
	ctx.r10.u64 = ctx.r20.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r20.u8 & 0x3F));
	// b 0x8295f438
	goto loc_8295F438;
loc_8295F5C0:
	// subf r11,r7,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r7.u64;
	// b 0x8295f434
	goto loc_8295F434;
loc_8295F5C8:
	// li r11,31
	ctx.r11.s64 = 31;
loc_8295F5CC:
	// rlwinm r9,r6,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r26,r26,-1
	ctx.r26.s64 = ctx.r26.s64 + -1;
	// addi r9,r9,3
	ctx.r9.s64 = ctx.r9.s64 + 3;
	// rlwinm r10,r10,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// stb r9,0(r26)
	REX_STORE_U8(ctx.r26.u32 + 0, ctx.r9.u8);
	// b 0x8295f4ac
	goto loc_8295F4AC;
loc_8295F5E4:
	// clrlwi r9,r10,31
	ctx.r9.u64 = ctx.r10.u32 & 0x1;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// bne cr6,0x8295f5cc
	if (!ctx.cr6.eq) goto loc_8295F5CC;
	// cmplw cr6,r11,r7
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r7.u32, ctx.xer);
	// rlwinm r9,r10,31,1,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// bge cr6,0x8295f624
	if (!ctx.cr6.lt) goto loc_8295F624;
	// lwz r8,0(r5)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// subf r20,r11,r7
	ctx.r20.u64 = ctx.r7.u64 - ctx.r11.u64;
	// subf r10,r7,r11
	ctx.r10.u64 = ctx.r11.u64 - ctx.r7.u64;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// slw r19,r8,r11
	ctx.r19.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 << (ctx.r11.u8 & 0x3F));
	// addi r11,r10,32
	ctx.r11.s64 = ctx.r10.s64 + 32;
	// or r9,r19,r9
	ctx.r9.u64 = ctx.r19.u64 | ctx.r9.u64;
	// srw r10,r8,r20
	ctx.r10.u64 = ctx.r20.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r20.u8 & 0x3F));
	// b 0x8295f488
	goto loc_8295F488;
loc_8295F624:
	// subf r11,r7,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r7.u64;
	// b 0x8295f484
	goto loc_8295F484;
loc_8295F62C:
	// li r11,31
	ctx.r11.s64 = 31;
loc_8295F630:
	// rlwinm r9,r6,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r26,r26,-1
	ctx.r26.s64 = ctx.r26.s64 + -1;
	// addi r9,r9,3
	ctx.r9.s64 = ctx.r9.s64 + 3;
	// rlwinm r10,r10,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// stb r9,0(r26)
	REX_STORE_U8(ctx.r26.u32 + 0, ctx.r9.u8);
	// b 0x8295f70c
	goto loc_8295F70C;
loc_8295F648:
	// clrlwi r9,r10,31
	ctx.r9.u64 = ctx.r10.u32 & 0x1;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// bne cr6,0x8295f630
	if (!ctx.cr6.eq) goto loc_8295F630;
	// cmplw cr6,r11,r7
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r7.u32, ctx.xer);
	// rlwinm r9,r10,31,1,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// bge cr6,0x8295f688
	if (!ctx.cr6.lt) goto loc_8295F688;
	// lwz r8,0(r5)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// subf r20,r11,r7
	ctx.r20.u64 = ctx.r7.u64 - ctx.r11.u64;
	// subf r10,r7,r11
	ctx.r10.u64 = ctx.r11.u64 - ctx.r7.u64;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// slw r19,r8,r11
	ctx.r19.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r8.u32 << (ctx.r11.u8 & 0x3F));
	// addi r11,r10,32
	ctx.r11.s64 = ctx.r10.s64 + 32;
	// or r9,r19,r9
	ctx.r9.u64 = ctx.r19.u64 | ctx.r9.u64;
	// srw r10,r8,r20
	ctx.r10.u64 = ctx.r20.u8 & 0x20 ? 0 : (ctx.r8.u32 >> (ctx.r20.u8 & 0x3F));
	// b 0x8295f4d8
	goto loc_8295F4D8;
loc_8295F688:
	// subf r11,r7,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r7.u64;
	// b 0x8295f4d4
	goto loc_8295F4D4;
loc_8295F690:
	// rlwinm r6,r8,30,2,31
	ctx.r6.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 30) & 0x3FFFFFFF;
	// cmplw cr6,r11,r7
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r7.u32, ctx.xer);
	// bge cr6,0x8295f6cc
	if (!ctx.cr6.lt) goto loc_8295F6CC;
	// lwz r9,0(r5)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// subfic r20,r7,32
	ctx.xer.ca = ctx.r7.u32 <= 32;
	ctx.r20.u64 = static_cast<uint64_t>(32) - ctx.r7.u64;
	// subf r19,r11,r7
	ctx.r19.u64 = ctx.r7.u64 - ctx.r11.u64;
	// subf r8,r7,r11
	ctx.r8.u64 = ctx.r11.u64 - ctx.r7.u64;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// slw r18,r9,r11
	ctx.r18.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r11.u8 & 0x3F));
	// srw r20,r28,r20
	ctx.r20.u64 = ctx.r20.u8 & 0x20 ? 0 : (ctx.r28.u32 >> (ctx.r20.u8 & 0x3F));
	// or r10,r18,r10
	ctx.r10.u64 = ctx.r18.u64 | ctx.r10.u64;
	// addi r11,r8,32
	ctx.r11.s64 = ctx.r8.s64 + 32;
	// and r8,r20,r10
	ctx.r8.u64 = ctx.r20.u64 & ctx.r10.u64;
	// srw r10,r9,r19
	ctx.r10.u64 = ctx.r19.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r19.u8 & 0x3F));
	// b 0x8295f6e0
	goto loc_8295F6E0;
loc_8295F6CC:
	// subfic r9,r7,32
	ctx.xer.ca = ctx.r7.u32 <= 32;
	ctx.r9.u64 = static_cast<uint64_t>(32) - ctx.r7.u64;
	// subf r11,r7,r11
	ctx.r11.u64 = ctx.r11.u64 - ctx.r7.u64;
	// srw r9,r28,r9
	ctx.r9.u64 = ctx.r9.u8 & 0x20 ? 0 : (ctx.r28.u32 >> (ctx.r9.u8 & 0x3F));
	// and r8,r9,r10
	ctx.r8.u64 = ctx.r9.u64 & ctx.r10.u64;
	// srw r10,r10,r7
	ctx.r10.u64 = ctx.r7.u8 & 0x20 ? 0 : (ctx.r10.u32 >> (ctx.r7.u8 & 0x3F));
loc_8295F6E0:
	// and r9,r8,r30
	ctx.r9.u64 = ctx.r8.u64 & ctx.r30.u64;
	// and r8,r8,r31
	ctx.r8.u64 = ctx.r8.u64 & ctx.r31.u64;
	// or r9,r9,r29
	ctx.r9.u64 = ctx.r9.u64 | ctx.r29.u64;
	// cmplwi cr6,r8,0
	ctx.cr6.compare<uint32_t>(ctx.r8.u32, 0, ctx.xer);
	// beq cr6,0x8295f6f8
	if (ctx.cr6.eq) goto loc_8295F6F8;
	// neg r9,r9
	ctx.r9.s64 = static_cast<int64_t>(-ctx.r9.u64);
loc_8295F6F8:
	// rlwinm r8,r6,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r6.u32 | (ctx.r6.u64 << 32), 1) & 0xFFFFFFFE;
	// stb r22,0(r3)
	REX_STORE_U8(ctx.r3.u32 + 0, ctx.r22.u8);
	// addi r6,r1,80
	ctx.r6.s64 = ctx.r1.s64 + 80;
	// sthx r9,r8,r6
	REX_STORE_U16(ctx.r8.u32 + ctx.r6.u32, ctx.r9.u16);
loc_8295F708:
	// addi r3,r3,1
	ctx.r3.s64 = ctx.r3.s64 + 1;
loc_8295F70C:
	// cmplw cr6,r3,r25
	ctx.cr6.compare<uint32_t>(ctx.r3.u32, ctx.r25.u32, ctx.xer);
	// blt cr6,0x8295f2f4
	if (ctx.cr6.lt) goto loc_8295F2F4;
loc_8295F714:
	// addi r7,r7,-1
	ctx.r7.s64 = ctx.r7.s64 + -1;
	// srawi r29,r29,1
	ctx.xer.ca = (ctx.r29.s32 < 0) & ((ctx.r29.u32 & 0x1) != 0);
	ctx.r29.s64 = ctx.r29.s32 >> 1;
	// cmplwi cr6,r7,1
	ctx.cr6.compare<uint32_t>(ctx.r7.u32, 1, ctx.xer);
	// bgt cr6,0x8295f2d8
	if (ctx.cr6.gt) goto loc_8295F2D8;
loc_8295F724:
	// cmplwi cr6,r27,0
	ctx.cr6.compare<uint32_t>(ctx.r27.u32, 0, ctx.xer);
	// beq cr6,0x8295fa88
	if (ctx.cr6.eq) goto loc_8295FA88;
	// mr r6,r26
	ctx.r6.u64 = ctx.r26.u64;
	// cmplw cr6,r26,r25
	ctx.cr6.compare<uint32_t>(ctx.r26.u32, ctx.r25.u32, ctx.xer);
	// bge cr6,0x8295fa88
	if (!ctx.cr6.lt) goto loc_8295FA88;
loc_8295F738:
	// lbz r8,0(r6)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r6.u32 + 0);
	// cmplwi cr6,r8,0
	ctx.cr6.compare<uint32_t>(ctx.r8.u32, 0, ctx.xer);
	// beq cr6,0x8295fa7c
	if (ctx.cr6.eq) goto loc_8295FA7C;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295f760
	if (!ctx.cr6.eq) goto loc_8295F760;
	// lwz r9,0(r5)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// rlwinm r10,r9,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295f76c
	goto loc_8295F76C;
loc_8295F760:
	// mr r9,r10
	ctx.r9.u64 = ctx.r10.u64;
	// rlwinm r10,r10,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295F76C:
	// clrlwi r9,r9,31
	ctx.r9.u64 = ctx.r9.u32 & 0x1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// beq cr6,0x8295fa7c
	if (ctx.cr6.eq) goto loc_8295FA7C;
	// clrlwi r9,r8,30
	ctx.r9.u64 = ctx.r8.u32 & 0x3;
	// cmplwi cr6,r9,3
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 3, ctx.xer);
	// bgt cr6,0x8295fa7c
	if (ctx.cr6.gt) goto loc_8295FA7C;
	// lis r12,-32106
	ctx.r12.s64 = -2104098816;
	// addi r12,r12,-2148
	ctx.r12.s64 = ctx.r12.s64 + -2148;
	// rlwinm r0,r9,2,0,29
	ctx.r0.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// lwzx r0,r12,r0
	ctx.r0.u64 = REX_LOAD_U32(ctx.r12.u32 + ctx.r0.u32);
	// mtctr r0
	ctx.ctr.u64 = ctx.r0.u64;
	// bctr 
	switch (ctx.r9.u32) {
	case 0:
		goto loc_8295F7AC;
	case 1:
		goto loc_8295F7C0;
	case 2:
		goto loc_8295F7F8;
	case 3:
		goto loc_8295FA34;
	default:
		__builtin_trap(); // Switch case out of range
	}
loc_8295F7AC:
	// rlwinm r7,r8,30,2,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 30) & 0x3FFFFFFF;
	// rlwinm r9,r7,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r9,r9,17
	ctx.r9.s64 = ctx.r9.s64 + 17;
	// stb r9,0(r6)
	REX_STORE_U8(ctx.r6.u32 + 0, ctx.r9.u8);
	// b 0x8295f804
	goto loc_8295F804;
loc_8295F7C0:
	// rlwinm r9,r8,0,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 0) & 0xFFFFFFFC;
	// addi r8,r9,2
	ctx.r8.s64 = ctx.r9.s64 + 2;
	// addi r7,r9,18
	ctx.r7.s64 = ctx.r9.s64 + 18;
	// addi r4,r9,34
	ctx.r4.s64 = ctx.r9.s64 + 34;
	// addi r9,r9,50
	ctx.r9.s64 = ctx.r9.s64 + 50;
	// stb r8,0(r6)
	REX_STORE_U8(ctx.r6.u32 + 0, ctx.r8.u8);
	// mr r8,r9
	ctx.r8.u64 = ctx.r9.u64;
	// addi r9,r25,1
	ctx.r9.s64 = ctx.r25.s64 + 1;
	// stb r7,0(r25)
	REX_STORE_U8(ctx.r25.u32 + 0, ctx.r7.u8);
	// stb r4,0(r9)
	REX_STORE_U8(ctx.r9.u32 + 0, ctx.r4.u8);
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// addi r25,r9,1
	ctx.r25.s64 = ctx.r9.s64 + 1;
	// stb r8,0(r9)
	REX_STORE_U8(ctx.r9.u32 + 0, ctx.r8.u8);
	// b 0x8295fa80
	goto loc_8295FA80;
loc_8295F7F8:
	// stb r22,0(r6)
	REX_STORE_U8(ctx.r6.u32 + 0, ctx.r22.u8);
	// rlwinm r7,r8,30,2,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 30) & 0x3FFFFFFF;
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
loc_8295F804:
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295f820
	if (!ctx.cr6.eq) goto loc_8295F820;
	// lwz r8,0(r5)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// li r9,31
	ctx.r9.s64 = 31;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// rlwinm r10,r8,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295f82c
	goto loc_8295F82C;
loc_8295F820:
	// mr r8,r10
	ctx.r8.u64 = ctx.r10.u64;
	// rlwinm r10,r10,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r9,r11,-1
	ctx.r9.s64 = ctx.r11.s64 + -1;
loc_8295F82C:
	// clrlwi r11,r8,31
	ctx.r11.u64 = ctx.r8.u32 & 0x1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq cr6,0x8295f84c
	if (ctx.cr6.eq) goto loc_8295F84C;
	// rlwinm r11,r7,2,0,29
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r26,r26,-1
	ctx.r26.s64 = ctx.r26.s64 + -1;
	// addi r11,r11,3
	ctx.r11.s64 = ctx.r11.s64 + 3;
	// stb r11,0(r26)
	REX_STORE_U8(ctx.r26.u32 + 0, ctx.r11.u8);
	// b 0x8295f88c
	goto loc_8295F88C;
loc_8295F84C:
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// bne cr6,0x8295f868
	if (!ctx.cr6.eq) goto loc_8295F868;
	// lwz r11,0(r5)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// li r9,31
	ctx.r9.s64 = 31;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// rlwinm r10,r11,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295f874
	goto loc_8295F874;
loc_8295F868:
	// mr r11,r10
	ctx.r11.u64 = ctx.r10.u64;
	// rlwinm r10,r10,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r9,r9,-1
	ctx.r9.s64 = ctx.r9.s64 + -1;
loc_8295F874:
	// clrlwi r11,r11,31
	ctx.r11.u64 = ctx.r11.u32 & 0x1;
	// rlwinm r8,r7,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 1) & 0xFFFFFFFE;
	// neg r11,r11
	ctx.r11.s64 = static_cast<int64_t>(-ctx.r11.u64);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// rlwimi r11,r23,0,31,15
	ctx.r11.u64 = (__builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 0) & 0xFFFFFFFFFFFF0001) | (ctx.r11.u64 & 0xFFFE);
	// sthx r11,r8,r4
	REX_STORE_U16(ctx.r8.u32 + ctx.r4.u32, ctx.r11.u16);
loc_8295F88C:
	// addi r7,r7,1
	ctx.r7.s64 = ctx.r7.s64 + 1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// bne cr6,0x8295f8ac
	if (!ctx.cr6.eq) goto loc_8295F8AC;
	// lwz r8,0(r5)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// li r10,31
	ctx.r10.s64 = 31;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// rlwinm r11,r8,31,1,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295f8b8
	goto loc_8295F8B8;
loc_8295F8AC:
	// mr r8,r10
	ctx.r8.u64 = ctx.r10.u64;
	// rlwinm r11,r10,31,1,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r10,r9,-1
	ctx.r10.s64 = ctx.r9.s64 + -1;
loc_8295F8B8:
	// clrlwi r9,r8,31
	ctx.r9.u64 = ctx.r8.u32 & 0x1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// beq cr6,0x8295f8d8
	if (ctx.cr6.eq) goto loc_8295F8D8;
	// rlwinm r9,r7,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r26,r26,-1
	ctx.r26.s64 = ctx.r26.s64 + -1;
	// addi r9,r9,3
	ctx.r9.s64 = ctx.r9.s64 + 3;
	// stb r9,0(r26)
	REX_STORE_U8(ctx.r26.u32 + 0, ctx.r9.u8);
	// b 0x8295f918
	goto loc_8295F918;
loc_8295F8D8:
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// bne cr6,0x8295f8f4
	if (!ctx.cr6.eq) goto loc_8295F8F4;
	// lwz r9,0(r5)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// li r10,31
	ctx.r10.s64 = 31;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// rlwinm r11,r9,31,1,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295f900
	goto loc_8295F900;
loc_8295F8F4:
	// mr r9,r11
	ctx.r9.u64 = ctx.r11.u64;
	// rlwinm r11,r11,31,1,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r10,r10,-1
	ctx.r10.s64 = ctx.r10.s64 + -1;
loc_8295F900:
	// clrlwi r9,r9,31
	ctx.r9.u64 = ctx.r9.u32 & 0x1;
	// rlwinm r8,r7,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 1) & 0xFFFFFFFE;
	// neg r9,r9
	ctx.r9.s64 = static_cast<int64_t>(-ctx.r9.u64);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// rlwimi r9,r23,0,31,15
	ctx.r9.u64 = (__builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 0) & 0xFFFFFFFFFFFF0001) | (ctx.r9.u64 & 0xFFFE);
	// sthx r9,r8,r4
	REX_STORE_U16(ctx.r8.u32 + ctx.r4.u32, ctx.r9.u16);
loc_8295F918:
	// addi r7,r7,1
	ctx.r7.s64 = ctx.r7.s64 + 1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// bne cr6,0x8295f938
	if (!ctx.cr6.eq) goto loc_8295F938;
	// lwz r8,0(r5)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// li r9,31
	ctx.r9.s64 = 31;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// rlwinm r11,r8,31,1,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295f944
	goto loc_8295F944;
loc_8295F938:
	// mr r8,r11
	ctx.r8.u64 = ctx.r11.u64;
	// rlwinm r11,r11,31,1,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r9,r10,-1
	ctx.r9.s64 = ctx.r10.s64 + -1;
loc_8295F944:
	// clrlwi r10,r8,31
	ctx.r10.u64 = ctx.r8.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x8295f964
	if (ctx.cr6.eq) goto loc_8295F964;
	// rlwinm r10,r7,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r26,r26,-1
	ctx.r26.s64 = ctx.r26.s64 + -1;
	// addi r10,r10,3
	ctx.r10.s64 = ctx.r10.s64 + 3;
	// stb r10,0(r26)
	REX_STORE_U8(ctx.r26.u32 + 0, ctx.r10.u8);
	// b 0x8295f9a4
	goto loc_8295F9A4;
loc_8295F964:
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// bne cr6,0x8295f980
	if (!ctx.cr6.eq) goto loc_8295F980;
	// lwz r10,0(r5)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// li r9,31
	ctx.r9.s64 = 31;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// rlwinm r11,r10,31,1,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295f98c
	goto loc_8295F98C;
loc_8295F980:
	// mr r10,r11
	ctx.r10.u64 = ctx.r11.u64;
	// rlwinm r11,r11,31,1,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r9,r9,-1
	ctx.r9.s64 = ctx.r9.s64 + -1;
loc_8295F98C:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// rlwinm r8,r7,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 1) & 0xFFFFFFFE;
	// neg r10,r10
	ctx.r10.s64 = static_cast<int64_t>(-ctx.r10.u64);
	// addi r4,r1,80
	ctx.r4.s64 = ctx.r1.s64 + 80;
	// rlwimi r10,r23,0,31,15
	ctx.r10.u64 = (__builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 0) & 0xFFFFFFFFFFFF0001) | (ctx.r10.u64 & 0xFFFE);
	// sthx r10,r8,r4
	REX_STORE_U16(ctx.r8.u32 + ctx.r4.u32, ctx.r10.u16);
loc_8295F9A4:
	// addi r7,r7,1
	ctx.r7.s64 = ctx.r7.s64 + 1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// bne cr6,0x8295f9c4
	if (!ctx.cr6.eq) goto loc_8295F9C4;
	// lwz r8,0(r5)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// rlwinm r10,r8,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295f9d0
	goto loc_8295F9D0;
loc_8295F9C4:
	// mr r8,r11
	ctx.r8.u64 = ctx.r11.u64;
	// rlwinm r10,r11,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r9,-1
	ctx.r11.s64 = ctx.r9.s64 + -1;
loc_8295F9D0:
	// clrlwi r9,r8,31
	ctx.r9.u64 = ctx.r8.u32 & 0x1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// beq cr6,0x8295f9f0
	if (ctx.cr6.eq) goto loc_8295F9F0;
	// rlwinm r9,r7,2,0,29
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r26,r26,-1
	ctx.r26.s64 = ctx.r26.s64 + -1;
	// addi r9,r9,3
	ctx.r9.s64 = ctx.r9.s64 + 3;
	// stb r9,0(r26)
	REX_STORE_U8(ctx.r26.u32 + 0, ctx.r9.u8);
	// b 0x8295fa80
	goto loc_8295FA80;
loc_8295F9F0:
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295fa0c
	if (!ctx.cr6.eq) goto loc_8295FA0C;
	// lwz r9,0(r5)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// rlwinm r10,r9,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295fa18
	goto loc_8295FA18;
loc_8295FA0C:
	// mr r9,r10
	ctx.r9.u64 = ctx.r10.u64;
	// rlwinm r10,r10,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295FA18:
	// clrlwi r9,r9,31
	ctx.r9.u64 = ctx.r9.u32 & 0x1;
	// rlwinm r8,r7,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 1) & 0xFFFFFFFE;
	// neg r9,r9
	ctx.r9.s64 = static_cast<int64_t>(-ctx.r9.u64);
	// addi r7,r1,80
	ctx.r7.s64 = ctx.r1.s64 + 80;
	// rlwimi r9,r23,0,31,15
	ctx.r9.u64 = (__builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 0) & 0xFFFFFFFFFFFF0001) | (ctx.r9.u64 & 0xFFFE);
	// sthx r9,r8,r7
	REX_STORE_U16(ctx.r8.u32 + ctx.r7.u32, ctx.r9.u16);
	// b 0x8295fa80
	goto loc_8295FA80;
loc_8295FA34:
	// rlwinm r8,r8,30,2,31
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 30) & 0x3FFFFFFF;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295fa54
	if (!ctx.cr6.eq) goto loc_8295FA54;
	// lwz r9,0(r5)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r5.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r5,r5,4
	ctx.r5.s64 = ctx.r5.s64 + 4;
	// rlwinm r10,r9,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295fa60
	goto loc_8295FA60;
loc_8295FA54:
	// mr r9,r10
	ctx.r9.u64 = ctx.r10.u64;
	// rlwinm r10,r10,31,1,31
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295FA60:
	// clrlwi r9,r9,31
	ctx.r9.u64 = ctx.r9.u32 & 0x1;
	// stb r22,0(r6)
	REX_STORE_U8(ctx.r6.u32 + 0, ctx.r22.u8);
	// rlwinm r8,r8,1,0,30
	ctx.r8.u64 = __builtin_rotateleft64(ctx.r8.u32 | (ctx.r8.u64 << 32), 1) & 0xFFFFFFFE;
	// neg r9,r9
	ctx.r9.s64 = static_cast<int64_t>(-ctx.r9.u64);
	// addi r7,r1,80
	ctx.r7.s64 = ctx.r1.s64 + 80;
	// rlwimi r9,r23,0,31,15
	ctx.r9.u64 = (__builtin_rotateleft64(ctx.r23.u32 | (ctx.r23.u64 << 32), 0) & 0xFFFFFFFFFFFF0001) | (ctx.r9.u64 & 0xFFFE);
	// sthx r9,r8,r7
	REX_STORE_U16(ctx.r8.u32 + ctx.r7.u32, ctx.r9.u16);
loc_8295FA7C:
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
loc_8295FA80:
	// cmplw cr6,r6,r25
	ctx.cr6.compare<uint32_t>(ctx.r6.u32, ctx.r25.u32, ctx.xer);
	// blt cr6,0x8295f738
	if (ctx.cr6.lt) goto loc_8295F738;
loc_8295FA88:
	// lhz r9,82(r1)
	ctx.r9.u64 = REX_LOAD_U16(ctx.r1.u32 + 82);
	// lwz r8,88(r1)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r1.u32 + 88);
	// stw r10,0(r21)
	REX_STORE_U32(ctx.r21.u32 + 0, ctx.r10.u32);
	// stw r11,8(r21)
	REX_STORE_U32(ctx.r21.u32 + 8, ctx.r11.u32);
	// stw r5,4(r21)
	REX_STORE_U32(ctx.r21.u32 + 4, ctx.r5.u32);
	// sth r9,2(r24)
	REX_STORE_U16(ctx.r24.u32 + 2, ctx.r9.u16);
	// stw r8,4(r24)
	REX_STORE_U32(ctx.r24.u32 + 4, ctx.r8.u32);
	// lwz r7,96(r1)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r1.u32 + 96);
	// lwz r6,104(r1)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r1.u32 + 104);
	// lwz r10,84(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 84);
	// lwz r11,92(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 92);
	// lwz r9,100(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 100);
	// lwz r8,108(r1)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r1.u32 + 108);
	// stw r7,8(r24)
	REX_STORE_U32(ctx.r24.u32 + 8, ctx.r7.u32);
	// stw r6,12(r24)
	REX_STORE_U32(ctx.r24.u32 + 12, ctx.r6.u32);
	// stw r10,16(r24)
	REX_STORE_U32(ctx.r24.u32 + 16, ctx.r10.u32);
	// stw r11,20(r24)
	REX_STORE_U32(ctx.r24.u32 + 20, ctx.r11.u32);
	// stw r9,24(r24)
	REX_STORE_U32(ctx.r24.u32 + 24, ctx.r9.u32);
	// stw r8,28(r24)
	REX_STORE_U32(ctx.r24.u32 + 28, ctx.r8.u32);
	// lwz r7,128(r1)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r1.u32 + 128);
	// lwz r6,168(r1)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r1.u32 + 168);
	// lwz r10,112(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 112);
	// lwz r11,120(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 120);
	// lwz r9,132(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 132);
	// lwz r8,172(r1)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r1.u32 + 172);
	// stw r7,32(r24)
	REX_STORE_U32(ctx.r24.u32 + 32, ctx.r7.u32);
	// stw r6,36(r24)
	REX_STORE_U32(ctx.r24.u32 + 36, ctx.r6.u32);
	// stw r10,40(r24)
	REX_STORE_U32(ctx.r24.u32 + 40, ctx.r10.u32);
	// stw r11,44(r24)
	REX_STORE_U32(ctx.r24.u32 + 44, ctx.r11.u32);
	// stw r9,48(r24)
	REX_STORE_U32(ctx.r24.u32 + 48, ctx.r9.u32);
	// stw r8,52(r24)
	REX_STORE_U32(ctx.r24.u32 + 52, ctx.r8.u32);
	// lwz r7,116(r1)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r1.u32 + 116);
	// lwz r6,124(r1)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r1.u32 + 124);
	// lwz r10,136(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 136);
	// lwz r11,144(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 144);
	// lwz r9,176(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 176);
	// lwz r8,184(r1)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r1.u32 + 184);
	// stw r7,56(r24)
	REX_STORE_U32(ctx.r24.u32 + 56, ctx.r7.u32);
	// stw r6,60(r24)
	REX_STORE_U32(ctx.r24.u32 + 60, ctx.r6.u32);
	// stw r10,64(r24)
	REX_STORE_U32(ctx.r24.u32 + 64, ctx.r10.u32);
	// stw r11,68(r24)
	REX_STORE_U32(ctx.r24.u32 + 68, ctx.r11.u32);
	// stw r9,72(r24)
	REX_STORE_U32(ctx.r24.u32 + 72, ctx.r9.u32);
	// stw r8,76(r24)
	REX_STORE_U32(ctx.r24.u32 + 76, ctx.r8.u32);
	// lwz r7,140(r1)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r1.u32 + 140);
	// lwz r6,148(r1)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r1.u32 + 148);
	// lwz r10,180(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 180);
	// lwz r11,188(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 188);
	// lwz r9,152(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 152);
	// lwz r8,160(r1)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r1.u32 + 160);
	// stw r7,80(r24)
	REX_STORE_U32(ctx.r24.u32 + 80, ctx.r7.u32);
	// stw r6,84(r24)
	REX_STORE_U32(ctx.r24.u32 + 84, ctx.r6.u32);
	// stw r10,88(r24)
	REX_STORE_U32(ctx.r24.u32 + 88, ctx.r10.u32);
	// stw r11,92(r24)
	REX_STORE_U32(ctx.r24.u32 + 92, ctx.r11.u32);
	// stw r9,96(r24)
	REX_STORE_U32(ctx.r24.u32 + 96, ctx.r9.u32);
	// stw r8,100(r24)
	REX_STORE_U32(ctx.r24.u32 + 100, ctx.r8.u32);
	// lwz r7,192(r1)
	ctx.r7.u64 = REX_LOAD_U32(ctx.r1.u32 + 192);
	// lwz r6,200(r1)
	ctx.r6.u64 = REX_LOAD_U32(ctx.r1.u32 + 200);
	// lwz r10,156(r1)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r1.u32 + 156);
	// lwz r11,164(r1)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r1.u32 + 164);
	// lwz r9,196(r1)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r1.u32 + 196);
	// lwz r8,204(r1)
	ctx.r8.u64 = REX_LOAD_U32(ctx.r1.u32 + 204);
	// stw r7,104(r24)
	REX_STORE_U32(ctx.r24.u32 + 104, ctx.r7.u32);
	// stw r6,108(r24)
	REX_STORE_U32(ctx.r24.u32 + 108, ctx.r6.u32);
	// stw r10,112(r24)
	REX_STORE_U32(ctx.r24.u32 + 112, ctx.r10.u32);
	// stw r11,116(r24)
	REX_STORE_U32(ctx.r24.u32 + 116, ctx.r11.u32);
	// stw r9,120(r24)
	REX_STORE_U32(ctx.r24.u32 + 120, ctx.r9.u32);
	// stw r8,124(r24)
	REX_STORE_U32(ctx.r24.u32 + 124, ctx.r8.u32);
	// addi r1,r1,464
	ctx.r1.s64 = ctx.r1.s64 + 464;
	// b 0x824131d0
	__restgprlr_18(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_8295FBA0) {
	REX_FUNC_PROLOGUE(sub_8295FBA0);
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413190
	ctx.lr = 0x8295FBA8;
	__savegprlr_22(ctx, base);
	// lwz r11,8(r4)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r4.u32 + 8);
	// li r25,0
	ctx.r25.s64 = 0;
	// lwz r10,0(r4)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r4.u32 + 0);
	// lwz r31,4(r4)
	ctx.r31.u64 = REX_LOAD_U32(ctx.r4.u32 + 4);
	// mr r30,r25
	ctx.r30.u64 = ctx.r25.u64;
	// cmplwi cr6,r11,3
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 3, ctx.xer);
	// bge cr6,0x8295fbec
	if (!ctx.cr6.lt) goto loc_8295FBEC;
	// lwz r9,0(r31)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// subfic r8,r11,3
	ctx.xer.ca = ctx.r11.u32 <= 3;
	ctx.r8.u64 = static_cast<uint64_t>(3) - ctx.r11.u64;
	// clrlwi r10,r10,24
	ctx.r10.u64 = ctx.r10.u32 & 0xFF;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// slw r7,r9,r11
	ctx.r7.u64 = ctx.r11.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r11.u8 & 0x3F));
	// addi r11,r11,29
	ctx.r11.s64 = ctx.r11.s64 + 29;
	// clrlwi r7,r7,24
	ctx.r7.u64 = ctx.r7.u32 & 0xFF;
	// or r10,r7,r10
	ctx.r10.u64 = ctx.r7.u64 | ctx.r10.u64;
	// srw r7,r9,r8
	ctx.r7.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r9.u32 >> (ctx.r8.u8 & 0x3F));
	// b 0x8295fbf4
	goto loc_8295FBF4;
loc_8295FBEC:
	// rlwinm r7,r10,29,3,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 29) & 0x1FFFFFFF;
	// addi r11,r11,-3
	ctx.r11.s64 = ctx.r11.s64 + -3;
loc_8295FBF4:
	// li r8,16
	ctx.r8.s64 = 16;
	// clrlwi r10,r10,29
	ctx.r10.u64 = ctx.r10.u32 & 0x7;
	// li r9,1
	ctx.r9.s64 = 1;
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
	// addi r28,r1,-236
	ctx.r28.s64 = ctx.r1.s64 + -236;
	// stb r8,-236(r1)
	REX_STORE_U8(ctx.r1.u32 + -236, ctx.r8.u8);
	// li r8,96
	ctx.r8.s64 = 96;
	// addi r26,r1,-232
	ctx.r26.s64 = ctx.r1.s64 + -232;
	// mr r29,r25
	ctx.r29.u64 = ctx.r25.u64;
	// mr r24,r10
	ctx.r24.u64 = ctx.r10.u64;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// stb r8,-235(r1)
	REX_STORE_U8(ctx.r1.u32 + -235, ctx.r8.u8);
	// li r8,176
	ctx.r8.s64 = 176;
	// stb r8,-234(r1)
	REX_STORE_U8(ctx.r1.u32 + -234, ctx.r8.u8);
	// li r8,2
	ctx.r8.s64 = 2;
	// stb r8,-233(r1)
	REX_STORE_U8(ctx.r1.u32 + -233, ctx.r8.u8);
	// addi r8,r10,-1
	ctx.r8.s64 = ctx.r10.s64 + -1;
	// slw r27,r9,r8
	ctx.r27.u64 = ctx.r8.u8 & 0x20 ? 0 : (ctx.r9.u32 << (ctx.r8.u8 & 0x3F));
	// beq cr6,0x829600ac
	if (ctx.cr6.eq) goto loc_829600AC;
loc_8295FC40:
	// mr r6,r25
	ctx.r6.u64 = ctx.r25.u64;
	// cmpwi cr6,r29,0
	ctx.cr6.compare<int32_t>(ctx.r29.s32, 0, ctx.xer);
	// ble cr6,0x8295fcc0
	if (!ctx.cr6.gt) goto loc_8295FCC0;
loc_8295FC4C:
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295fc68
	if (!ctx.cr6.eq) goto loc_8295FC68;
	// lwz r10,0(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295fc74
	goto loc_8295FC74;
loc_8295FC68:
	// mr r10,r7
	ctx.r10.u64 = ctx.r7.u64;
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295FC74:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x8295fcb4
	if (ctx.cr6.eq) goto loc_8295FCB4;
	// addi r10,r1,-160
	ctx.r10.s64 = ctx.r1.s64 + -160;
	// neg r8,r27
	ctx.r8.s64 = static_cast<int64_t>(-ctx.r27.u64);
	// lbzx r10,r6,r10
	ctx.r10.u64 = REX_LOAD_U8(ctx.r6.u32 + ctx.r10.u32);
	// lbzx r9,r10,r3
	ctx.r9.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r3.u32);
	// cmplwi cr6,r9,128
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 128, ctx.xer);
	// bge cr6,0x8295fc9c
	if (!ctx.cr6.lt) goto loc_8295FC9C;
	// mr r8,r27
	ctx.r8.u64 = ctx.r27.u64;
loc_8295FC9C:
	// lbzx r9,r10,r3
	ctx.r9.u64 = REX_LOAD_U8(ctx.r10.u32 + ctx.r3.u32);
	// cmplw cr6,r30,r5
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, ctx.r5.u32, ctx.xer);
	// addi r30,r30,1
	ctx.r30.s64 = ctx.r30.s64 + 1;
	// add r9,r9,r8
	ctx.r9.u64 = ctx.r9.u64 + ctx.r8.u64;
	// stbx r9,r10,r3
	REX_STORE_U8(ctx.r10.u32 + ctx.r3.u32, ctx.r9.u8);
	// beq cr6,0x829600ac
	if (ctx.cr6.eq) goto loc_829600AC;
loc_8295FCB4:
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
	// cmpw cr6,r6,r29
	ctx.cr6.compare<int32_t>(ctx.r6.s32, ctx.r29.s32, ctx.xer);
	// blt cr6,0x8295fc4c
	if (ctx.cr6.lt) goto loc_8295FC4C;
loc_8295FCC0:
	// mr r6,r28
	ctx.r6.u64 = ctx.r28.u64;
	// cmplw cr6,r28,r26
	ctx.cr6.compare<uint32_t>(ctx.r28.u32, ctx.r26.u32, ctx.xer);
	// bge cr6,0x8296009c
	if (!ctx.cr6.lt) goto loc_8296009C;
	// addi r10,r1,-160
	ctx.r10.s64 = ctx.r1.s64 + -160;
	// add r8,r29,r10
	ctx.r8.u64 = ctx.r29.u64 + ctx.r10.u64;
loc_8295FCD4:
	// lbz r10,0(r6)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r6.u32 + 0);
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x82960090
	if (ctx.cr6.eq) goto loc_82960090;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295fcfc
	if (!ctx.cr6.eq) goto loc_8295FCFC;
	// lwz r9,0(r31)
	ctx.r9.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// rlwinm r7,r9,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295fd08
	goto loc_8295FD08;
loc_8295FCFC:
	// mr r9,r7
	ctx.r9.u64 = ctx.r7.u64;
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295FD08:
	// clrlwi r9,r9,31
	ctx.r9.u64 = ctx.r9.u32 & 0x1;
	// cmplwi cr6,r9,0
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 0, ctx.xer);
	// beq cr6,0x82960090
	if (ctx.cr6.eq) goto loc_82960090;
	// clrlwi r9,r10,30
	ctx.r9.u64 = ctx.r10.u32 & 0x3;
	// cmplwi cr6,r9,3
	ctx.cr6.compare<uint32_t>(ctx.r9.u32, 3, ctx.xer);
	// bgt cr6,0x82960090
	if (ctx.cr6.gt) goto loc_82960090;
	// lis r12,-32106
	ctx.r12.s64 = -2104098816;
	// addi r12,r12,-712
	ctx.r12.s64 = ctx.r12.s64 + -712;
	// rlwinm r0,r9,2,0,29
	ctx.r0.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// lwzx r0,r12,r0
	ctx.r0.u64 = REX_LOAD_U32(ctx.r12.u32 + ctx.r0.u32);
	// mtctr r0
	ctx.ctr.u64 = ctx.r0.u64;
	// bctr 
	switch (ctx.r9.u32) {
	case 0:
		goto loc_8295FD48;
	case 1:
		goto loc_8295FD5C;
	case 2:
		goto loc_8295FD94;
	case 3:
		goto loc_82960030;
	default:
		__builtin_trap(); // Switch case out of range
	}
loc_8295FD48:
	// rlwinm r9,r10,30,2,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 30) & 0x3FFFFFFF;
	// rlwinm r10,r9,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r10,r10,17
	ctx.r10.s64 = ctx.r10.s64 + 17;
	// stb r10,0(r6)
	REX_STORE_U8(ctx.r6.u32 + 0, ctx.r10.u8);
	// b 0x8295fda0
	goto loc_8295FDA0;
loc_8295FD5C:
	// rlwinm r10,r10,0,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 0) & 0xFFFFFFFC;
	// addi r9,r10,2
	ctx.r9.s64 = ctx.r10.s64 + 2;
	// addi r23,r10,18
	ctx.r23.s64 = ctx.r10.s64 + 18;
	// addi r22,r10,34
	ctx.r22.s64 = ctx.r10.s64 + 34;
	// addi r10,r10,50
	ctx.r10.s64 = ctx.r10.s64 + 50;
	// stb r9,0(r6)
	REX_STORE_U8(ctx.r6.u32 + 0, ctx.r9.u8);
	// mr r9,r10
	ctx.r9.u64 = ctx.r10.u64;
	// addi r10,r26,1
	ctx.r10.s64 = ctx.r26.s64 + 1;
	// stb r23,0(r26)
	REX_STORE_U8(ctx.r26.u32 + 0, ctx.r23.u8);
	// stb r22,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r22.u8);
	// addi r10,r10,1
	ctx.r10.s64 = ctx.r10.s64 + 1;
	// addi r26,r10,1
	ctx.r26.s64 = ctx.r10.s64 + 1;
	// stb r9,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r9.u8);
	// b 0x82960094
	goto loc_82960094;
loc_8295FD94:
	// stb r25,0(r6)
	REX_STORE_U8(ctx.r6.u32 + 0, ctx.r25.u8);
	// rlwinm r9,r10,30,2,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 30) & 0x3FFFFFFF;
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
loc_8295FDA0:
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295fdbc
	if (!ctx.cr6.eq) goto loc_8295FDBC;
	// lwz r10,0(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295fdc8
	goto loc_8295FDC8;
loc_8295FDBC:
	// mr r10,r7
	ctx.r10.u64 = ctx.r7.u64;
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295FDC8:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x8295fde8
	if (ctx.cr6.eq) goto loc_8295FDE8;
	// rlwinm r10,r9,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r28,r28,-1
	ctx.r28.s64 = ctx.r28.s64 + -1;
	// addi r10,r10,3
	ctx.r10.s64 = ctx.r10.s64 + 3;
	// stb r10,0(r28)
	REX_STORE_U8(ctx.r28.u32 + 0, ctx.r10.u8);
	// b 0x8295fe40
	goto loc_8295FE40;
loc_8295FDE8:
	// stb r9,0(r8)
	REX_STORE_U8(ctx.r8.u32 + 0, ctx.r9.u8);
	// addi r29,r29,1
	ctx.r29.s64 = ctx.r29.s64 + 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// addi r8,r8,1
	ctx.r8.s64 = ctx.r8.s64 + 1;
	// bne cr6,0x8295fe10
	if (!ctx.cr6.eq) goto loc_8295FE10;
	// lwz r10,0(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295fe1c
	goto loc_8295FE1C;
loc_8295FE10:
	// mr r10,r7
	ctx.r10.u64 = ctx.r7.u64;
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295FE1C:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// neg r10,r27
	ctx.r10.s64 = static_cast<int64_t>(-ctx.r27.u64);
	// bne cr6,0x8295fe30
	if (!ctx.cr6.eq) goto loc_8295FE30;
	// mr r10,r27
	ctx.r10.u64 = ctx.r27.u64;
loc_8295FE30:
	// cmplw cr6,r30,r5
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, ctx.r5.u32, ctx.xer);
	// stbx r10,r9,r3
	REX_STORE_U8(ctx.r9.u32 + ctx.r3.u32, ctx.r10.u8);
	// addi r30,r30,1
	ctx.r30.s64 = ctx.r30.s64 + 1;
	// beq cr6,0x829600ac
	if (ctx.cr6.eq) goto loc_829600AC;
loc_8295FE40:
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295fe60
	if (!ctx.cr6.eq) goto loc_8295FE60;
	// lwz r10,0(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295fe6c
	goto loc_8295FE6C;
loc_8295FE60:
	// mr r10,r7
	ctx.r10.u64 = ctx.r7.u64;
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295FE6C:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x8295fe8c
	if (ctx.cr6.eq) goto loc_8295FE8C;
	// rlwinm r10,r9,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r28,r28,-1
	ctx.r28.s64 = ctx.r28.s64 + -1;
	// addi r10,r10,3
	ctx.r10.s64 = ctx.r10.s64 + 3;
	// stb r10,0(r28)
	REX_STORE_U8(ctx.r28.u32 + 0, ctx.r10.u8);
	// b 0x8295fee4
	goto loc_8295FEE4;
loc_8295FE8C:
	// stb r9,0(r8)
	REX_STORE_U8(ctx.r8.u32 + 0, ctx.r9.u8);
	// addi r29,r29,1
	ctx.r29.s64 = ctx.r29.s64 + 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// addi r8,r8,1
	ctx.r8.s64 = ctx.r8.s64 + 1;
	// bne cr6,0x8295feb4
	if (!ctx.cr6.eq) goto loc_8295FEB4;
	// lwz r10,0(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295fec0
	goto loc_8295FEC0;
loc_8295FEB4:
	// mr r10,r7
	ctx.r10.u64 = ctx.r7.u64;
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295FEC0:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// neg r10,r27
	ctx.r10.s64 = static_cast<int64_t>(-ctx.r27.u64);
	// bne cr6,0x8295fed4
	if (!ctx.cr6.eq) goto loc_8295FED4;
	// mr r10,r27
	ctx.r10.u64 = ctx.r27.u64;
loc_8295FED4:
	// cmplw cr6,r30,r5
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, ctx.r5.u32, ctx.xer);
	// stbx r10,r9,r3
	REX_STORE_U8(ctx.r9.u32 + ctx.r3.u32, ctx.r10.u8);
	// addi r30,r30,1
	ctx.r30.s64 = ctx.r30.s64 + 1;
	// beq cr6,0x829600ac
	if (ctx.cr6.eq) goto loc_829600AC;
loc_8295FEE4:
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295ff04
	if (!ctx.cr6.eq) goto loc_8295FF04;
	// lwz r10,0(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295ff10
	goto loc_8295FF10;
loc_8295FF04:
	// mr r10,r7
	ctx.r10.u64 = ctx.r7.u64;
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295FF10:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x8295ff30
	if (ctx.cr6.eq) goto loc_8295FF30;
	// rlwinm r10,r9,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r28,r28,-1
	ctx.r28.s64 = ctx.r28.s64 + -1;
	// addi r10,r10,3
	ctx.r10.s64 = ctx.r10.s64 + 3;
	// stb r10,0(r28)
	REX_STORE_U8(ctx.r28.u32 + 0, ctx.r10.u8);
	// b 0x8295ff88
	goto loc_8295FF88;
loc_8295FF30:
	// stb r9,0(r8)
	REX_STORE_U8(ctx.r8.u32 + 0, ctx.r9.u8);
	// addi r29,r29,1
	ctx.r29.s64 = ctx.r29.s64 + 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// addi r8,r8,1
	ctx.r8.s64 = ctx.r8.s64 + 1;
	// bne cr6,0x8295ff58
	if (!ctx.cr6.eq) goto loc_8295FF58;
	// lwz r10,0(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295ff64
	goto loc_8295FF64;
loc_8295FF58:
	// mr r10,r7
	ctx.r10.u64 = ctx.r7.u64;
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295FF64:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// neg r10,r27
	ctx.r10.s64 = static_cast<int64_t>(-ctx.r27.u64);
	// bne cr6,0x8295ff78
	if (!ctx.cr6.eq) goto loc_8295FF78;
	// mr r10,r27
	ctx.r10.u64 = ctx.r27.u64;
loc_8295FF78:
	// cmplw cr6,r30,r5
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, ctx.r5.u32, ctx.xer);
	// stbx r10,r9,r3
	REX_STORE_U8(ctx.r9.u32 + ctx.r3.u32, ctx.r10.u8);
	// addi r30,r30,1
	ctx.r30.s64 = ctx.r30.s64 + 1;
	// beq cr6,0x829600ac
	if (ctx.cr6.eq) goto loc_829600AC;
loc_8295FF88:
	// addi r9,r9,1
	ctx.r9.s64 = ctx.r9.s64 + 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne cr6,0x8295ffa8
	if (!ctx.cr6.eq) goto loc_8295FFA8;
	// lwz r10,0(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x8295ffb4
	goto loc_8295FFB4;
loc_8295FFA8:
	// mr r10,r7
	ctx.r10.u64 = ctx.r7.u64;
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_8295FFB4:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// beq cr6,0x8295ffd4
	if (ctx.cr6.eq) goto loc_8295FFD4;
	// rlwinm r10,r9,2,0,29
	ctx.r10.u64 = __builtin_rotateleft64(ctx.r9.u32 | (ctx.r9.u64 << 32), 2) & 0xFFFFFFFC;
	// addi r28,r28,-1
	ctx.r28.s64 = ctx.r28.s64 + -1;
	// addi r10,r10,3
	ctx.r10.s64 = ctx.r10.s64 + 3;
	// stb r10,0(r28)
	REX_STORE_U8(ctx.r28.u32 + 0, ctx.r10.u8);
	// b 0x82960094
	goto loc_82960094;
loc_8295FFD4:
	// stb r9,0(r8)
	REX_STORE_U8(ctx.r8.u32 + 0, ctx.r9.u8);
	// addi r29,r29,1
	ctx.r29.s64 = ctx.r29.s64 + 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// addi r8,r8,1
	ctx.r8.s64 = ctx.r8.s64 + 1;
	// bne cr6,0x8295fffc
	if (!ctx.cr6.eq) goto loc_8295FFFC;
	// lwz r10,0(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x82960008
	goto loc_82960008;
loc_8295FFFC:
	// mr r10,r7
	ctx.r10.u64 = ctx.r7.u64;
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_82960008:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// neg r10,r27
	ctx.r10.s64 = static_cast<int64_t>(-ctx.r27.u64);
	// bne cr6,0x8296001c
	if (!ctx.cr6.eq) goto loc_8296001C;
	// mr r10,r27
	ctx.r10.u64 = ctx.r27.u64;
loc_8296001C:
	// cmplw cr6,r30,r5
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, ctx.r5.u32, ctx.xer);
	// stbx r10,r9,r3
	REX_STORE_U8(ctx.r9.u32 + ctx.r3.u32, ctx.r10.u8);
	// addi r30,r30,1
	ctx.r30.s64 = ctx.r30.s64 + 1;
	// beq cr6,0x829600ac
	if (ctx.cr6.eq) goto loc_829600AC;
	// b 0x82960094
	goto loc_82960094;
loc_82960030:
	// rlwinm r9,r10,30,2,31
	ctx.r9.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 30) & 0x3FFFFFFF;
	// addi r29,r29,1
	ctx.r29.s64 = ctx.r29.s64 + 1;
	// cmplwi cr6,r11,0
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// stb r9,0(r8)
	REX_STORE_U8(ctx.r8.u32 + 0, ctx.r9.u8);
	// addi r8,r8,1
	ctx.r8.s64 = ctx.r8.s64 + 1;
	// bne cr6,0x8296005c
	if (!ctx.cr6.eq) goto loc_8296005C;
	// lwz r10,0(r31)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r31.u32 + 0);
	// li r11,31
	ctx.r11.s64 = 31;
	// addi r31,r31,4
	ctx.r31.s64 = ctx.r31.s64 + 4;
	// rlwinm r7,r10,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r10.u32 | (ctx.r10.u64 << 32), 31) & 0x7FFFFFFF;
	// b 0x82960068
	goto loc_82960068;
loc_8296005C:
	// mr r10,r7
	ctx.r10.u64 = ctx.r7.u64;
	// rlwinm r7,r7,31,1,31
	ctx.r7.u64 = __builtin_rotateleft64(ctx.r7.u32 | (ctx.r7.u64 << 32), 31) & 0x7FFFFFFF;
	// addi r11,r11,-1
	ctx.r11.s64 = ctx.r11.s64 + -1;
loc_82960068:
	// clrlwi r10,r10,31
	ctx.r10.u64 = ctx.r10.u32 & 0x1;
	// cmplwi cr6,r10,0
	ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
	// neg r10,r27
	ctx.r10.s64 = static_cast<int64_t>(-ctx.r27.u64);
	// bne cr6,0x8296007c
	if (!ctx.cr6.eq) goto loc_8296007C;
	// mr r10,r27
	ctx.r10.u64 = ctx.r27.u64;
loc_8296007C:
	// cmplw cr6,r30,r5
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, ctx.r5.u32, ctx.xer);
	// stbx r10,r9,r3
	REX_STORE_U8(ctx.r9.u32 + ctx.r3.u32, ctx.r10.u8);
	// addi r30,r30,1
	ctx.r30.s64 = ctx.r30.s64 + 1;
	// beq cr6,0x829600ac
	if (ctx.cr6.eq) goto loc_829600AC;
	// stb r25,0(r6)
	REX_STORE_U8(ctx.r6.u32 + 0, ctx.r25.u8);
loc_82960090:
	// addi r6,r6,1
	ctx.r6.s64 = ctx.r6.s64 + 1;
loc_82960094:
	// cmplw cr6,r6,r26
	ctx.cr6.compare<uint32_t>(ctx.r6.u32, ctx.r26.u32, ctx.xer);
	// blt cr6,0x8295fcd4
	if (ctx.cr6.lt) goto loc_8295FCD4;
loc_8296009C:
	// addi r24,r24,-1
	ctx.r24.s64 = ctx.r24.s64 + -1;
	// srawi r27,r27,1
	ctx.xer.ca = (ctx.r27.s32 < 0) & ((ctx.r27.u32 & 0x1) != 0);
	ctx.r27.s64 = ctx.r27.s32 >> 1;
	// cmplwi cr6,r24,0
	ctx.cr6.compare<uint32_t>(ctx.r24.u32, 0, ctx.xer);
	// bne cr6,0x8295fc40
	if (!ctx.cr6.eq) goto loc_8295FC40;
loc_829600AC:
	// stw r31,4(r4)
	REX_STORE_U32(ctx.r4.u32 + 4, ctx.r31.u32);
	// stw r7,0(r4)
	REX_STORE_U32(ctx.r4.u32 + 0, ctx.r7.u32);
	// stw r11,8(r4)
	REX_STORE_U32(ctx.r4.u32 + 8, ctx.r11.u32);
	// b 0x824131e0
	__restgprlr_22(ctx, base);
	return;
}

DEFINE_REX_FUNC(sub_829600C0) {
	REX_FUNC_PROLOGUE(sub_829600C0);
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x82413198
	ctx.lr = 0x829600C8;
	__savegprlr_24(ctx, base);
	// stwu r1,-224(r1)
	ea = -224 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// li r9,0
	ctx.r9.s64 = 0;
	// mr r29,r4
	ctx.r29.u64 = ctx.r4.u64;
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// mr r4,r5
	ctx.r4.u64 = ctx.r5.u64;
	// mr r30,r7
	ctx.r30.u64 = ctx.r7.u64;
	// mr r28,r8
	ctx.r28.u64 = ctx.r8.u64;
	// std r9,80(r1)
	REX_STORE_U64(ctx.r1.u32 + 80, ctx.r9.u64);
	// addi r11,r1,88
	ctx.r11.s64 = ctx.r1.s64 + 88;
	// li r10,7
	ctx.r10.s64 = 7;
	// mtctr r10
	ctx.ctr.u64 = ctx.r10.u64;
loc_829600F4:
	// std r9,0(r11)
	REX_STORE_U64(ctx.r11.u32 + 0, ctx.r9.u64);
	// addi r11,r11,8
	ctx.r11.s64 = ctx.r11.s64 + 8;
	// bdnz 0x829600f4
	--ctx.ctr.u64;
	if (ctx.ctr.u32 != 0) goto loc_829600F4;
	// mr r5,r6
	ctx.r5.u64 = ctx.r6.u64;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// bl 0x8295fba0
	ctx.lr = 0x8296010C;
	sub_8295FBA0(ctx, base);
	// lbz r11,0(r30)
	ctx.r11.u64 = REX_LOAD_U8(ctx.r30.u32 + 0);
	// lbz r10,80(r1)
	ctx.r10.u64 = REX_LOAD_U8(ctx.r1.u32 + 80);
	// lbz r7,81(r1)
	ctx.r7.u64 = REX_LOAD_U8(ctx.r1.u32 + 81);
	// add r10,r11,r10
	ctx.r10.u64 = ctx.r11.u64 + ctx.r10.u64;
	// lbz r24,84(r1)
	ctx.r24.u64 = REX_LOAD_U8(ctx.r1.u32 + 84);
	// lbz r25,85(r1)
	ctx.r25.u64 = REX_LOAD_U8(ctx.r1.u32 + 85);
	// add r11,r30,r28
	ctx.r11.u64 = ctx.r30.u64 + ctx.r28.u64;
	// lbz r26,88(r1)
	ctx.r26.u64 = REX_LOAD_U8(ctx.r1.u32 + 88);
	// lbz r27,89(r1)
	ctx.r27.u64 = REX_LOAD_U8(ctx.r1.u32 + 89);
	// lbz r3,92(r1)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r1.u32 + 92);
	// stb r10,0(r31)
	REX_STORE_U8(ctx.r31.u32 + 0, ctx.r10.u8);
	// add r10,r31,r29
	ctx.r10.u64 = ctx.r31.u64 + ctx.r29.u64;
	// lbz r8,1(r30)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r30.u32 + 1);
	// lbz r4,93(r1)
	ctx.r4.u64 = REX_LOAD_U8(ctx.r1.u32 + 93);
	// add r5,r8,r7
	ctx.r5.u64 = ctx.r8.u64 + ctx.r7.u64;
	// lbz r9,82(r1)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r1.u32 + 82);
	// lbz r6,83(r1)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r1.u32 + 83);
	// lbz r7,86(r1)
	ctx.r7.u64 = REX_LOAD_U8(ctx.r1.u32 + 86);
	// lbz r8,87(r1)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r1.u32 + 87);
	// stb r5,1(r31)
	REX_STORE_U8(ctx.r31.u32 + 1, ctx.r5.u8);
	// lbz r5,2(r30)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r30.u32 + 2);
	// add r5,r5,r24
	ctx.r5.u64 = ctx.r5.u64 + ctx.r24.u64;
	// stb r5,2(r31)
	REX_STORE_U8(ctx.r31.u32 + 2, ctx.r5.u8);
	// lbz r5,3(r30)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r30.u32 + 3);
	// add r5,r5,r25
	ctx.r5.u64 = ctx.r5.u64 + ctx.r25.u64;
	// lbz r25,94(r1)
	ctx.r25.u64 = REX_LOAD_U8(ctx.r1.u32 + 94);
	// stb r5,3(r31)
	REX_STORE_U8(ctx.r31.u32 + 3, ctx.r5.u8);
	// lbz r5,4(r30)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r30.u32 + 4);
	// add r5,r5,r26
	ctx.r5.u64 = ctx.r5.u64 + ctx.r26.u64;
	// lbz r26,95(r1)
	ctx.r26.u64 = REX_LOAD_U8(ctx.r1.u32 + 95);
	// stb r5,4(r31)
	REX_STORE_U8(ctx.r31.u32 + 4, ctx.r5.u8);
	// lbz r5,5(r30)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r30.u32 + 5);
	// add r5,r5,r27
	ctx.r5.u64 = ctx.r5.u64 + ctx.r27.u64;
	// stb r5,5(r31)
	REX_STORE_U8(ctx.r31.u32 + 5, ctx.r5.u8);
	// lbz r5,6(r30)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r30.u32 + 6);
	// add r5,r5,r3
	ctx.r5.u64 = ctx.r5.u64 + ctx.r3.u64;
	// lbz r3,125(r1)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r1.u32 + 125);
	// stb r5,6(r31)
	REX_STORE_U8(ctx.r31.u32 + 6, ctx.r5.u8);
	// lbz r5,7(r30)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r30.u32 + 7);
	// lbz r30,105(r1)
	ctx.r30.u64 = REX_LOAD_U8(ctx.r1.u32 + 105);
	// add r5,r5,r4
	ctx.r5.u64 = ctx.r5.u64 + ctx.r4.u64;
	// lbz r4,96(r1)
	ctx.r4.u64 = REX_LOAD_U8(ctx.r1.u32 + 96);
	// stb r5,7(r31)
	REX_STORE_U8(ctx.r31.u32 + 7, ctx.r5.u8);
	// lbz r5,0(r11)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// lbz r31,124(r1)
	ctx.r31.u64 = REX_LOAD_U8(ctx.r1.u32 + 124);
	// add r9,r9,r5
	ctx.r9.u64 = ctx.r9.u64 + ctx.r5.u64;
	// lbz r5,97(r1)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r1.u32 + 97);
	// stb r9,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r9.u8);
	// lbz r9,1(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 1);
	// add r9,r9,r6
	ctx.r9.u64 = ctx.r9.u64 + ctx.r6.u64;
	// lbz r6,100(r1)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r1.u32 + 100);
	// stb r9,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r9.u8);
	// lbz r9,2(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 2);
	// add r9,r9,r7
	ctx.r9.u64 = ctx.r9.u64 + ctx.r7.u64;
	// lbz r7,91(r1)
	ctx.r7.u64 = REX_LOAD_U8(ctx.r1.u32 + 91);
	// stb r9,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r9.u8);
	// lbz r9,3(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 3);
	// add r9,r9,r8
	ctx.r9.u64 = ctx.r9.u64 + ctx.r8.u64;
	// lbz r8,90(r1)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r1.u32 + 90);
	// stb r9,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r9.u8);
	// lbz r9,4(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 4);
	// add r9,r9,r8
	ctx.r9.u64 = ctx.r9.u64 + ctx.r8.u64;
	// lbz r8,104(r1)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r1.u32 + 104);
	// stb r9,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r9.u8);
	// lbz r9,5(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 5);
	// add r27,r9,r7
	ctx.r27.u64 = ctx.r9.u64 + ctx.r7.u64;
	// lbz r7,101(r1)
	ctx.r7.u64 = REX_LOAD_U8(ctx.r1.u32 + 101);
	// lbz r9,106(r1)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r1.u32 + 106);
	// stb r27,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r27.u8);
	// lbz r27,6(r11)
	ctx.r27.u64 = REX_LOAD_U8(ctx.r11.u32 + 6);
	// add r27,r27,r25
	ctx.r27.u64 = ctx.r27.u64 + ctx.r25.u64;
	// stb r27,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r27.u8);
	// lbz r27,7(r11)
	ctx.r27.u64 = REX_LOAD_U8(ctx.r11.u32 + 7);
	// add r11,r11,r28
	ctx.r11.u64 = ctx.r11.u64 + ctx.r28.u64;
	// add r27,r27,r26
	ctx.r27.u64 = ctx.r27.u64 + ctx.r26.u64;
	// stb r27,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r27.u8);
	// add r10,r10,r29
	ctx.r10.u64 = ctx.r10.u64 + ctx.r29.u64;
	// lbz r27,0(r11)
	ctx.r27.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// add r8,r8,r27
	ctx.r8.u64 = ctx.r8.u64 + ctx.r27.u64;
	// lbz r25,127(r1)
	ctx.r25.u64 = REX_LOAD_U8(ctx.r1.u32 + 127);
	// lbz r26,98(r1)
	ctx.r26.u64 = REX_LOAD_U8(ctx.r1.u32 + 98);
	// lbz r27,99(r1)
	ctx.r27.u64 = REX_LOAD_U8(ctx.r1.u32 + 99);
	// stb r8,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r8.u8);
	// lbz r8,1(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 1);
	// add r8,r8,r30
	ctx.r8.u64 = ctx.r8.u64 + ctx.r30.u64;
	// lbz r30,102(r1)
	ctx.r30.u64 = REX_LOAD_U8(ctx.r1.u32 + 102);
	// stb r8,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r8.u8);
	// lbz r8,2(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 2);
	// add r8,r8,r31
	ctx.r8.u64 = ctx.r8.u64 + ctx.r31.u64;
	// lbz r31,103(r1)
	ctx.r31.u64 = REX_LOAD_U8(ctx.r1.u32 + 103);
	// stb r8,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r8.u8);
	// lbz r8,3(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 3);
	// add r8,r8,r3
	ctx.r8.u64 = ctx.r8.u64 + ctx.r3.u64;
	// lbz r3,126(r1)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r1.u32 + 126);
	// stb r8,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r8.u8);
	// lbz r8,4(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 4);
	// add r8,r8,r4
	ctx.r8.u64 = ctx.r8.u64 + ctx.r4.u64;
	// lbz r4,109(r1)
	ctx.r4.u64 = REX_LOAD_U8(ctx.r1.u32 + 109);
	// stb r8,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r8.u8);
	// lbz r8,5(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 5);
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// lbz r5,112(r1)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r1.u32 + 112);
	// stb r8,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r8.u8);
	// lbz r8,6(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 6);
	// add r8,r8,r6
	ctx.r8.u64 = ctx.r8.u64 + ctx.r6.u64;
	// lbz r6,113(r1)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r1.u32 + 113);
	// stb r8,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r8.u8);
	// lbz r8,7(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 7);
	// add r11,r11,r28
	ctx.r11.u64 = ctx.r11.u64 + ctx.r28.u64;
	// add r8,r8,r7
	ctx.r8.u64 = ctx.r8.u64 + ctx.r7.u64;
	// lbz r7,128(r1)
	ctx.r7.u64 = REX_LOAD_U8(ctx.r1.u32 + 128);
	// stb r8,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r8.u8);
	// add r10,r10,r29
	ctx.r10.u64 = ctx.r10.u64 + ctx.r29.u64;
	// lbz r8,0(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// add r9,r9,r8
	ctx.r9.u64 = ctx.r9.u64 + ctx.r8.u64;
	// lbz r8,107(r1)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r1.u32 + 107);
	// stb r9,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r9.u8);
	// lbz r9,1(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 1);
	// add r8,r9,r8
	ctx.r8.u64 = ctx.r9.u64 + ctx.r8.u64;
	// lbz r9,108(r1)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r1.u32 + 108);
	// stb r8,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r8.u8);
	// lbz r8,2(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 2);
	// add r3,r8,r3
	ctx.r3.u64 = ctx.r8.u64 + ctx.r3.u64;
	// lbz r8,129(r1)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r1.u32 + 129);
	// stb r3,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r3.u8);
	// lbz r3,3(r11)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r11.u32 + 3);
	// add r3,r3,r25
	ctx.r3.u64 = ctx.r3.u64 + ctx.r25.u64;
	// stb r3,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r3.u8);
	// lbz r3,4(r11)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r11.u32 + 4);
	// add r3,r3,r26
	ctx.r3.u64 = ctx.r3.u64 + ctx.r26.u64;
	// stb r3,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r3.u8);
	// lbz r3,5(r11)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r11.u32 + 5);
	// add r3,r3,r27
	ctx.r3.u64 = ctx.r3.u64 + ctx.r27.u64;
	// stb r3,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r3.u8);
	// lbz r3,6(r11)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r11.u32 + 6);
	// add r3,r3,r30
	ctx.r3.u64 = ctx.r3.u64 + ctx.r30.u64;
	// stb r3,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r3.u8);
	// lbz r3,7(r11)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r11.u32 + 7);
	// add r11,r11,r28
	ctx.r11.u64 = ctx.r11.u64 + ctx.r28.u64;
	// add r3,r3,r31
	ctx.r3.u64 = ctx.r3.u64 + ctx.r31.u64;
	// stb r3,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r3.u8);
	// add r10,r10,r29
	ctx.r10.u64 = ctx.r10.u64 + ctx.r29.u64;
	// lbz r3,0(r11)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// add r9,r9,r3
	ctx.r9.u64 = ctx.r9.u64 + ctx.r3.u64;
	// stb r9,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r9.u8);
	// lbz r9,1(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 1);
	// add r9,r9,r4
	ctx.r9.u64 = ctx.r9.u64 + ctx.r4.u64;
	// stb r9,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r9.u8);
	// lbz r9,2(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 2);
	// add r9,r9,r5
	ctx.r9.u64 = ctx.r9.u64 + ctx.r5.u64;
	// stb r9,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r9.u8);
	// lbz r9,3(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 3);
	// add r9,r9,r6
	ctx.r9.u64 = ctx.r9.u64 + ctx.r6.u64;
	// stb r9,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r9.u8);
	// lbz r9,4(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 4);
	// add r9,r9,r7
	ctx.r9.u64 = ctx.r9.u64 + ctx.r7.u64;
	// stb r9,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r9.u8);
	// lbz r9,5(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 5);
	// add r9,r9,r8
	ctx.r9.u64 = ctx.r9.u64 + ctx.r8.u64;
	// stb r9,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r9.u8);
	// lbz r9,6(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 6);
	// lbz r7,132(r1)
	ctx.r7.u64 = REX_LOAD_U8(ctx.r1.u32 + 132);
	// lbz r8,133(r1)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r1.u32 + 133);
	// add r9,r9,r7
	ctx.r9.u64 = ctx.r9.u64 + ctx.r7.u64;
	// lbz r6,110(r1)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r1.u32 + 110);
	// lbz r26,111(r1)
	ctx.r26.u64 = REX_LOAD_U8(ctx.r1.u32 + 111);
	// lbz r27,114(r1)
	ctx.r27.u64 = REX_LOAD_U8(ctx.r1.u32 + 114);
	// lbz r30,115(r1)
	ctx.r30.u64 = REX_LOAD_U8(ctx.r1.u32 + 115);
	// lbz r31,130(r1)
	ctx.r31.u64 = REX_LOAD_U8(ctx.r1.u32 + 130);
	// stb r9,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r9.u8);
	// lbz r9,7(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 7);
	// add r11,r11,r28
	ctx.r11.u64 = ctx.r11.u64 + ctx.r28.u64;
	// lbz r3,131(r1)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r1.u32 + 131);
	// add r25,r9,r8
	ctx.r25.u64 = ctx.r9.u64 + ctx.r8.u64;
	// lbz r4,134(r1)
	ctx.r4.u64 = REX_LOAD_U8(ctx.r1.u32 + 134);
	// lbz r5,135(r1)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r1.u32 + 135);
	// lbz r9,116(r1)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r1.u32 + 116);
	// lbz r7,117(r1)
	ctx.r7.u64 = REX_LOAD_U8(ctx.r1.u32 + 117);
	// lbz r8,120(r1)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r1.u32 + 120);
	// stb r25,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r25.u8);
	// add r10,r10,r29
	ctx.r10.u64 = ctx.r10.u64 + ctx.r29.u64;
	// lbz r25,0(r11)
	ctx.r25.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// add r6,r6,r25
	ctx.r6.u64 = ctx.r6.u64 + ctx.r25.u64;
	// lbz r25,137(r1)
	ctx.r25.u64 = REX_LOAD_U8(ctx.r1.u32 + 137);
	// stb r6,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r6.u8);
	// lbz r6,1(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 1);
	// add r6,r6,r26
	ctx.r6.u64 = ctx.r6.u64 + ctx.r26.u64;
	// lbz r26,140(r1)
	ctx.r26.u64 = REX_LOAD_U8(ctx.r1.u32 + 140);
	// stb r6,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r6.u8);
	// lbz r6,2(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 2);
	// add r6,r6,r27
	ctx.r6.u64 = ctx.r6.u64 + ctx.r27.u64;
	// lbz r27,141(r1)
	ctx.r27.u64 = REX_LOAD_U8(ctx.r1.u32 + 141);
	// stb r6,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r6.u8);
	// lbz r6,3(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 3);
	// add r6,r6,r30
	ctx.r6.u64 = ctx.r6.u64 + ctx.r30.u64;
	// lbz r30,136(r1)
	ctx.r30.u64 = REX_LOAD_U8(ctx.r1.u32 + 136);
	// stb r6,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r6.u8);
	// lbz r6,4(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 4);
	// add r6,r6,r31
	ctx.r6.u64 = ctx.r6.u64 + ctx.r31.u64;
	// lbz r31,119(r1)
	ctx.r31.u64 = REX_LOAD_U8(ctx.r1.u32 + 119);
	// stb r6,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r6.u8);
	// lbz r6,5(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 5);
	// add r6,r6,r3
	ctx.r6.u64 = ctx.r6.u64 + ctx.r3.u64;
	// lbz r3,122(r1)
	ctx.r3.u64 = REX_LOAD_U8(ctx.r1.u32 + 122);
	// stb r6,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r6.u8);
	// lbz r6,6(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 6);
	// add r6,r6,r4
	ctx.r6.u64 = ctx.r6.u64 + ctx.r4.u64;
	// lbz r4,123(r1)
	ctx.r4.u64 = REX_LOAD_U8(ctx.r1.u32 + 123);
	// stb r6,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r6.u8);
	// lbz r6,7(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 7);
	// add r11,r11,r28
	ctx.r11.u64 = ctx.r11.u64 + ctx.r28.u64;
	// add r6,r6,r5
	ctx.r6.u64 = ctx.r6.u64 + ctx.r5.u64;
	// lbz r5,138(r1)
	ctx.r5.u64 = REX_LOAD_U8(ctx.r1.u32 + 138);
	// stb r6,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r6.u8);
	// add r10,r10,r29
	ctx.r10.u64 = ctx.r10.u64 + ctx.r29.u64;
	// lbz r6,0(r11)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// add r9,r9,r6
	ctx.r9.u64 = ctx.r9.u64 + ctx.r6.u64;
	// lbz r6,139(r1)
	ctx.r6.u64 = REX_LOAD_U8(ctx.r1.u32 + 139);
	// stb r9,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r9.u8);
	// lbz r9,1(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 1);
	// add r9,r9,r7
	ctx.r9.u64 = ctx.r9.u64 + ctx.r7.u64;
	// lbz r7,142(r1)
	ctx.r7.u64 = REX_LOAD_U8(ctx.r1.u32 + 142);
	// stb r9,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r9.u8);
	// lbz r9,2(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 2);
	// add r9,r9,r8
	ctx.r9.u64 = ctx.r9.u64 + ctx.r8.u64;
	// lbz r8,121(r1)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r1.u32 + 121);
	// stb r9,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r9.u8);
	// lbz r9,3(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 3);
	// add r9,r9,r8
	ctx.r9.u64 = ctx.r9.u64 + ctx.r8.u64;
	// lbz r8,118(r1)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r1.u32 + 118);
	// stb r9,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r9.u8);
	// lbz r9,4(r11)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r11.u32 + 4);
	// add r30,r9,r30
	ctx.r30.u64 = ctx.r9.u64 + ctx.r30.u64;
	// lbz r9,143(r1)
	ctx.r9.u64 = REX_LOAD_U8(ctx.r1.u32 + 143);
	// stb r30,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r30.u8);
	// lbz r30,5(r11)
	ctx.r30.u64 = REX_LOAD_U8(ctx.r11.u32 + 5);
	// add r30,r30,r25
	ctx.r30.u64 = ctx.r30.u64 + ctx.r25.u64;
	// stb r30,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r30.u8);
	// lbz r30,6(r11)
	ctx.r30.u64 = REX_LOAD_U8(ctx.r11.u32 + 6);
	// add r30,r30,r26
	ctx.r30.u64 = ctx.r30.u64 + ctx.r26.u64;
	// stb r30,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r30.u8);
	// lbz r30,7(r11)
	ctx.r30.u64 = REX_LOAD_U8(ctx.r11.u32 + 7);
	// add r11,r11,r28
	ctx.r11.u64 = ctx.r11.u64 + ctx.r28.u64;
	// add r30,r30,r27
	ctx.r30.u64 = ctx.r30.u64 + ctx.r27.u64;
	// stb r30,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r30.u8);
	// add r10,r10,r29
	ctx.r10.u64 = ctx.r10.u64 + ctx.r29.u64;
	// lbz r30,0(r11)
	ctx.r30.u64 = REX_LOAD_U8(ctx.r11.u32 + 0);
	// add r8,r8,r30
	ctx.r8.u64 = ctx.r8.u64 + ctx.r30.u64;
	// stb r8,0(r10)
	REX_STORE_U8(ctx.r10.u32 + 0, ctx.r8.u8);
	// lbz r8,1(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 1);
	// add r8,r8,r31
	ctx.r8.u64 = ctx.r8.u64 + ctx.r31.u64;
	// stb r8,1(r10)
	REX_STORE_U8(ctx.r10.u32 + 1, ctx.r8.u8);
	// lbz r8,2(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 2);
	// add r8,r8,r3
	ctx.r8.u64 = ctx.r8.u64 + ctx.r3.u64;
	// stb r8,2(r10)
	REX_STORE_U8(ctx.r10.u32 + 2, ctx.r8.u8);
	// lbz r8,3(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 3);
	// add r8,r8,r4
	ctx.r8.u64 = ctx.r8.u64 + ctx.r4.u64;
	// stb r8,3(r10)
	REX_STORE_U8(ctx.r10.u32 + 3, ctx.r8.u8);
	// lbz r8,4(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 4);
	// add r8,r8,r5
	ctx.r8.u64 = ctx.r8.u64 + ctx.r5.u64;
	// stb r8,4(r10)
	REX_STORE_U8(ctx.r10.u32 + 4, ctx.r8.u8);
	// lbz r8,5(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 5);
	// add r8,r8,r6
	ctx.r8.u64 = ctx.r8.u64 + ctx.r6.u64;
	// stb r8,5(r10)
	REX_STORE_U8(ctx.r10.u32 + 5, ctx.r8.u8);
	// lbz r8,6(r11)
	ctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + 6);
	// add r8,r8,r7
	ctx.r8.u64 = ctx.r8.u64 + ctx.r7.u64;
	// stb r8,6(r10)
	REX_STORE_U8(ctx.r10.u32 + 6, ctx.r8.u8);
	// lbz r11,7(r11)
	ctx.r11.u64 = REX_LOAD_U8(ctx.r11.u32 + 7);
	// add r11,r11,r9
	ctx.r11.u64 = ctx.r11.u64 + ctx.r9.u64;
	// stb r11,7(r10)
	REX_STORE_U8(ctx.r10.u32 + 7, ctx.r11.u8);
	// addi r1,r1,224
	ctx.r1.s64 = ctx.r1.s64 + 224;
	// b 0x824131e8
	__restgprlr_24(ctx, base);
	return;
}

