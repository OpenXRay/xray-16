----------------------------------------------------------------------------
-- LuaJIT x86 disassembler module.
--
-- Copyright (C) 2005-2008 Mike Pall. All rights reserved.
-- Released under the MIT/X license. See luajit.h for full copyright notice.
----------------------------------------------------------------------------
-- This is a helper module used by the LuaJIT machine code dumper module.
--
-- Sending small code snippets to an external disassembler and mixing the
-- output with our own stuff was too fragile. So I had to bite the bullet
-- and write yet another x86 disassembler. Oh well ...
--
-- The output format is very similar to what ndisasm generates. But it has
-- been developed independently by looking at the opcode tables from the
-- Intel and AMD manuals. The supported instruction set is quite extensive
-- and reflects what a current generation P4 or K8 implements in 32 bit
-- mode. Yes, this includes MMX, SSE, SSE2, SSE3, SSSE3 and even privileged
-- instructions.
--
-- Notes:
-- * The (useless) a16 prefix, 3DNow and pre-586 opcodes are unsupported.
-- * No attempt at optimization has been made -- it's fast enough for my needs.
-- * The public API may change when more architectures are added.
--
-- TODO:
-- * More testing with arbitrary x86 code (not just LuaJIT generated code).
-- * The output for a few MMX/SSE opcodes could be improved.
-- * Adding x64 support would be straightforward.
-- * Better input API (iterator) and output API (structured access to instr).
------------------------------------------------------------------------------

local type = type
local sub, byte, format = string.sub, string.byte, string.format
local match, gmatch, gsub = string.match, string.gmatch, string.gsub

-- Map for 1st opcode byte. Ugly? Well ... read on.
local map_opc1 = {
--0x
[0]="addBmr","addVmr","addBrm","addVrm","addBai","addVai","push es","pop es",
"orBmr","orVmr","orBrm","orVrm","orBai","orVai","push cs","opc2*",
--1x
"adcBmr","adcVmr","adcBrm","adcVrm","adcBai","adcVai","push ss","pop ss",
"sbbBmr","sbbVmr","sbbBrm","sbbVrm","sbbBai","sbbVai","push ds","pop ds",
--2x
"andBmr","andVmr","andBrm","andVrm","andBai","andVai","es:seg","daa",
"subBmr","subVmr","subBrm","subVrm","subBai","subVai","cs:seg","das",
--3x
"xorBmr","xorVmr","xorBrm","xorVrm","xorBai","xorVai","ss:seg","aaa",
"cmpBmr","cmpVmr","cmpBrm","cmpVrm","cmpBai","cmpVai","ds:seg","aas",
--4x
"incVR","incVR","incVR","incVR","incVR","incVR","incVR","incVR",
"decVR","decVR","decVR","decVR","decVR","decVR","decVR","decVR",
--5x
"pushVR","pushVR","pushVR","pushVR","pushVR","pushVR","pushVR","pushVR",
"popVR","popVR","popVR","popVR","popVR","popVR","popVR","popVR",
--6x
"pusha/pushaw","popa/popaw","boundVrm","arplWmr",
"fs:seg","gs:seg","o16:","a16",
"pushVi","imulVrmi","pushBs","imulVrms",
"insb","insd/insw","outsb","outsd/outsw",
--7x
"joBj","jnoBj","jbBj","jnbBj","jzBj","jnzBj","jbeBj","jaBj",
"jsBj","jnsBj","jpeBj","jpoBj","jlBj","jgeBj","jleBj","jgBj",
--8x
"arith!Bmi","arith!Vmi","arith!Bmi","arith!Vms",
"testBmr","testVmr","xchgBrm","xchgVrm",
"movBmr","movVmr","movBrm","movVrm",
"movVmg","leaVrm","movWgm","popVm",
--9x
"nop|pause|xchgWaR|repne nop","xchgVaR","xchgVaR","xchgVaR",
"xchgVaR","xchgVaR","xchgVaR","xchgVaR",
"cwde/cbw","cdq/cwd","call farViw","wait",
"pushf/pushfw","popf/popfw","sahf","lahf",
--Ax
"movBao","movVao","movBoa","movVoa",
"movsb","movsd/movsb","cmpsb","cmpsd/cmpsw",
"testBai","testVai","stosb","stosd/stosw",
"lodsb","lodsd/lodsw","scasb","scasd/scasw",
--Bx
"movBRi","movBRi","movBRi","movBRi","movBRi","movBRi","movBRi","movBRi",
"movVRi","movVRi","movVRi","movVRi","movVRi","movVRi","movVRi","movVRi",
--Cx
"shift!Bmu","shift!Vmu","retBw","ret","lesVrm","ldsVrm","movBmi","movVmi",
"enterBwu","leave","retfBw","retf","int3","intBu","into","iret/iretw",
--Dx
"shift!Bm1","shift!Vm1","shift!Bmc","shift!Vmc","aamBu","aadBu","salc","xlatb",
"fp*0","fp*1","fp*2","fp*3","fp*4","fp*5","fp*6","fp*7",
--Ex
"loopneBj","loopeBj","loopBj","jecxz/jcxzBj","inBau","inVau","outBua","outVua",
"callDj","jmpDj","jmp farViw","jmpBj","inBad","inVad","outBda","outVda",
--Fx
"lock:","int1","repne:rep","rep:","hlt","cmc","testb!Bm","testv!Vm",
"clc","stc","cli","sti","cld","std","inc!Bm","inc!Vm",
}
assert(#map_opc1 == 255)

-- Map for 2nd opcode byte (0f xx). True CISC hell. Hey, I told you.
-- Prefix dependent MMX/SSE opcodes: (none)|rep|o16|repne
local map_opc2 = {
--0x
[0]="sldt!Dmp","sgdt!Dmp","larVrm","lslVrm",nil,"syscall","clts","sysret",
"invd","wbinvd",nil,"ud1",nil,"prefetch!Bm","femms","3dnowMrmu",
--1x
"movupsXrm|movssXrm|movupdXrm|movsdXrm",
"movupsXmr|movssXmr|movupdXmr|movsdXmr",
"movhlpsXrm|movsldupXrm|movlpdXrm|movddupXrm", -- TODO: movlpsXrMm (mem case).
"movlpsXmr||movlpdXmr",
"unpcklpsXrm||unpcklpdXrm",
"unpckhpsXrm||unpckhpdXrm",
"movlhpsXrm|movshdupXrm|movhpdXrm", -- TODO: movhpsXrMm (mem case).
"movhpsXmr||movhpdXmr",
"prefetcht!Bm","hintnopBm","hintnopBm","hintnopBm",
"hintnopBm","hintnopBm","hintnopBm","hintnopBm",
--2x
"movDmx","movDmy","movDxm","movDym","movDmz",nil,"movDzm",nil,
"movapsXrm||movapdXrm",
"movapsXmr||movapdXmr",
"cvtpi2psXrMm|cvtsi2ssXrDm|cvtpi2pdXrMm|cvtsi2sdXrDm",
"movntpsXmr||movntpdXmr",
"cvttps2piMrXm|cvttss2siDrXm|cvttpd2piMrXm|cvttsd2siDrXm",
"cvtps2piMrXm|cvtss2siDrXm|cvtpd2piMrXm|cvtsd2siDrXm",
"ucomissXrm||ucomisdXrm",
"comissXrm||comisdXrm",
--3x
"wrmsr","rdtsc","rdmsr","rdpmc","sysenter","sysexit",nil,nil,
"ssse3*38",nil,"ssse3*3a",nil,nil,nil,nil,nil,
--4x
"cmovoVrm","cmovnoVrm","cmovbVrm","cmovnbVrm",
"cmovzVrm","cmovnzVrm","cmovbeVrm","cmovaVrm",
"cmovsVrm","cmovnsVrm","cmovpeVrm","cmovpoVrm",
"cmovlVrm","cmovgeVrm","cmovleVrm","cmovgVrm",
--5x
"movmskpsDrXm||movmskpdDrXm","sqrtpsXrm|sqrtssXrm|sqrtpdXrm|sqrtsdXrm",
"rsqrtpsXrm|rsqrtssXrm","rcppsXrm|rcpssXrm",
"andpsXrm||andpdXrm","andnpsXrm||andnpdXrm",
"orpsXrm||orpdXrm","xorpsXrm||xorpdXrm",
"addpsXrm|addssXrm|addpdXrm|addsdXrm","mulpsXrm|mulssXrm|mulpdXrm|mulsdXrm",
"cvtps2pdXrm|cvtss2sdXrm|cvtpd2psXrm|cvtsd2ssXrm",
"cvtdq2psXrm|cvttps2dqXrm|cvtps2dqXrm",
"subpsXrm|subssXrm|subpdXrm|subsdXrm","minpsXrm|minssXrm|minpdXrm|minsdXrm",
"divpsXrm|divssXrm|divpdXrm|divsdXrm","maxpsXrm|maxssXrm|maxpdXrm|maxsdXrm",
--6x
"punpcklbwMrm||punpcklbqXrm","punpcklwdPrm","punpckldqPrm","packsswbPrm",
"pcmpgtbPrm","pcmpgtwPrm","pcmpgtdPrm","packuswbPrm",
"punpckhbwPrm","punpckhwdPrm","punpckhdqPrm","packssdwPrm",
"||punpcklqdqXrm","||punpckhqdqXrm",
"movdPrDm","movqMrm|movdquXrm|movdqaXrm",
--7x
"pshufwPrmu","pshiftw!Pmu","pshiftd!Pmu","pshiftq!Mmu||pshiftdq!Xmu",
"pcmpeqbPrm","pcmpeqwPrm","pcmpeqdPrm","emms|",
nil,nil,nil,nil,
"||haddpdXrm|haddpsXrm","||hsubpdXrm|hsubpsXrm",
"movdDmMr|movqXrm|movdDmXr","movqMmr|movdquXmr|movdqaXmr",
--8x
"joVj","jnoVj","jbVj","jnbVj","jzVj","jnzVj","jbeVj","jaVj",
"jsVj","jnsVj","jpeVj","jpoVj","jlVj","jgeVj","jleVj","jgVj",
--9x
"setoBm","setnoBm","setbBm","setnbBm","setzBm","setnzBm","setbeBm","setaBm",
"setsBm","setnsBm","setpeBm","setpoBm","setlBm","setgeBm","setleBm","setgBm",
--Ax
"push fs","pop fs","cpuid","btVmr","shldVmru","shldVmrc",nil,nil,
"push gs","pop gs","rsm","btsVmr","shrdVmru","shrdVmrc","fxsave!Dmp","imulVrm",
--Bx
"cmpxchgBmr","cmpxchgVmr","lssVrm","btrVmr",
"lfsVrm","lgsVrm","movzxVrBm","movzxDrWm",
nil,"ud2","bt!Vmu","btcVmr",
"bsfVrm","bsrVrm","movsxVrBm","movsxDrWm",
--Cx
"xaddBmr","xaddVmr",
"cmppsXrmu|cmpssXrmu|cmppdXrmu|cmpsdXrmu","movntiDmr|",
"pinsrwPrWmu","pextrwDrPmu",
"shufpsXrmu||shufpdXrmu","cmpxchg!Dmp",
"bswapDR","bswapDR","bswapDR","bswapDR","bswapDR","bswapDR","bswapDR","bswapDR",
--Dx
"||addsubpdXrm|addsubpsXrm","psrlwPrm","psrldPrm","psrlqPrm",
"paddqPrm","pmullwPrm",
"|movq2dqXrMm|movqXmr|movdq2qMrXm","pmovmskbDrPm",
"psubusbPrm","psubuswPrm","pminubPrm","pandPrm",
"paddusbPrm","padduswPrm","pmaxubPrm","pandnPrm",
--Ex
"pavgbPrm","psrawPrm","psradPrm","pavgwPrm",
"pmulhuwPrm","pmulhwPrm",
"|cvtdq2pdXrm|cvttpd2dqXrm|cvtpd2dqXrm","movntqMmr||movntdqXmr",
"psubsbPrm","psubswPrm","pminswPrm","porPrm",
"paddsbPrm","paddswPrm","pmaxswPrm","pxorPrm",
--Fx
"|||lddquXrm","psllwPrm","pslldPrm","psllqPrm",
"pmuludqPrm","pmaddwdPrm","psadbwPrm","maskmovqMrm||maskmovdquXrm",
"psubbPrm","psubwPrm","psubdPrm","psubqPrm",
"paddbPrm","paddwPrm","padddPrm","ud",
}
assert(map_opc2[255] == "ud")

-- Map for SSSE3 opcodes.
local map_ssse3 = {
["38"] = { -- [66] 0f 38 xx
--0x
[0]="pshufbPrm","phaddwPrm","phadddPrm","phaddswPrm",
"pmaddubswPrm","phsubwPrm","phsubdPrm","phsubswPrm",
"psignbPrm","psignwPrm","psigndPrm","pmulhrswPrm",
nil,nil,nil,nil,
--1x
nil,nil,nil,nil,nil,nil,nil,nil,
nil,nil,nil,nil,"pabsbPrm","pabswPrm","pabsdPrm",nil,
},
["3a"] = { -- [66] 0f 3a xx
[0x0f] = "palignrPrmu",
},
}

-- Map for FP opcodes. And you thought stack machines are simple?
local map_opcfp = {
-- D8-DF 00-BF: opcodes with a memory operand.
-- D8
[0]="faddFm","fmulFm","fcomFm","fcompFm","fsubFm","fsubrFm","fdivFm","fdivrFm",
"fldFm",nil,"fstFm","fstpFm","fldenvDmp","fldcwWm","fnstenvDmp","fnstcwWm",
-- DA
"fiaddDm","fimulDm","ficomDm","ficompDm",
"fisubDm","fisubrDm","fidivDm","fidivrDm",
-- DB
"fildDm","fisttpDm","fistDm","fistpDm",nil,"fld twordFmp",nil,"fstp twordFmp",
-- DC
"faddGm","fmulGm","fcomGm","fcompGm","fsubGm","fsubrGm","fdivGm","fdivrGm",
-- DD
"fldGm","fisttpQm","fstGm","fstpGm","frstorDmp",nil,"fnsaveDmp","fnstswWm",
-- DE
"fiaddWm","fimulWm","ficomWm","ficompWm",
"fisubWm","fisubrWm","fidivWm","fidivrWm",
-- DF
"fildWm","fisttpWm","fistWm","fistpWm",
"fbld twordFmp","fildQm","fbstp twordFmp","fistpQm",
-- xx C0-FF: opcodes with a pseudo-register operand.
-- D8
"faddFf","fmulFf","fcomFf","fcompFf","fsubFf","fsubrFf","fdivFf","fdivrFf",
-- D9
"fldFf","fxchFf",{"fnop"},nil,
{"fchs","fabs",nil,nil,"ftst","fxam"},
{"fld1","fldl2t","fldl2e","fldpi","fldlg2","fldln2","fldz"},
{"f2xm1","fyl2x","fptan","fpatan","fxtract","fprem1","fdecstp","fincstp"},
{"fprem","fyl2xp1","fsqrt","fsincos","frndint","fscale","fsin","fcos"},
-- DA
"fcmovbFf","fcmoveFf","fcmovbeFf","fcmovuFf",nil,{nil,"fucompp"},nil,nil,
-- DB
"fcmovnbFf","fcmovneFf","fcmovnbeFf","fcmovnuFf",
{nil,nil,"fnclex","fninit"},"fucomiFf","fcomiFf",nil,
-- DC
"fadd toFf","fmul toFf",nil,nil,
"fsub toFf","fsubr toFf","fdivr toFf","fdiv toFf",
-- DD
"ffreeFf",nil,"fstFf","fstpFf","fucomFf","fucompFf",nil,nil,
-- DE
"faddpFf","fmulpFf",nil,{nil,"fcompp"},
"fsubrpFf","fsubpFf","fdivrpFf","fdivpFf",
-- DF
nil,nil,nil,nil,{"fnstsw ax"},"fucomipFf","fcomipFf",nil,
}
assert(map_opcfp[126] == "fcomipFf")

-- Map for opcode groups. The subkey is sp from the ModRM byte.
local map_opcgroup = {
  arith = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" },
  shift = { "rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar" },
  testb = { "testBmi", "testBmi", "not", "neg", "mul", "imul", "div", "idiv" },
  testv = { "testVmi", "testVmi", "not", "neg", "mul", "imul", "div", "idiv" },
  inc = { "inc", "dec", "callDmp", "call farDmp",
	  "jmpDmp", "jmp farDmp", "push" },
  sldt = { "sldt", "str", "lldt", "ltr", "verr", "verw" },
  sgdt = { "sgdt", "sidt", "lgdt", "lidt", "smsw", nil, "lmsw", "invlpg" },
  bt = { nil, nil, nil, nil, "bt", "bts", "btr", "btc" },
  cmpxchg = { nil, "cmpxchg8b" },
  pshiftw = { nil, nil, "psrlw", nil, "psraw", nil, "psllw" },
  pshiftd = { nil, nil, "psrld", nil, "psrad", nil, "pslld" },
  pshiftq = { nil, nil, "psrlq", nil, nil, nil, "psllq" },
  pshiftdq = { nil, nil, "psrlq", "psrldq", nil, nil, "psllq", "pslldq" },
  fxsave = { "fxsave", "fxrstor", "ldmxcsr", "stmxcsr",
	     nil, "lfenceDp", "mfenceDp", "sfenceDp" }, -- TODO: clflush.
  prefetch = { "prefetch", "prefetchw" },
  prefetcht = { "prefetchnta", "prefetcht0", "prefetcht1", "prefetcht2" },
}

------------------------------------------------------------------------------

-- Maps for register names.
local map_aregs = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" }
local map_regs = {
  B = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" },
  W = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" },
  D = map_aregs,
  M = { "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7" },
  X = { "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7" },
}
local map_segregs = { "es", "cs", "ss", "ds", "fs", "gs", "segr6", "segr7" }

-- Maps for size names.
local map_sz2n = {
  B = 1, W = 2, D = 4, Q = 8, M = 8, X = 16,
}
local map_sz2prefix = {
  B = "byte", W = "word", D = "dword",
  Q = "qword", -- No associated reg in 32 bit mode.
  F = "dword", G = "qword", -- No need for sizes/register names for these two.
  M = "qword", X = "xword",
}

------------------------------------------------------------------------------

-- Output a nicely formatted line with an opcode and operands.
local function putop(ctx, text, operands)
  local code, pos, hex = ctx.code, ctx.pos, ""
  for i=ctx.start,pos-1 do
    hex = hex..format("%02X", byte(code, i, i))
  end
  if #hex > 16 then hex = sub(hex, 1, 16).."." end
  if operands then text = text.." "..operands end
  if ctx.o16 then text = "o16 "..text; ctx.o16 = false end
  if ctx.rep then text = ctx.rep.." "..text; ctx.rep = false end
  if ctx.seg then
    local text2, n = gsub(text, "%[", "["..ctx.seg..":")
    if n == 0 then text = ctx.seg.." "..text else text = text2 end
    ctx.seg = false
  end
  if ctx.lock then text = "lock "..text; ctx.lock = false end
  local imm = ctx.imm
  if imm then
    local sym = ctx.symtab[imm]
    if sym then text = text.."\t->"..sym end
  end
  ctx.out(format("%08x  %-18s%s\n", ctx.addr+ctx.start, hex, text))
  ctx.mrm = false
  ctx.start = pos
  ctx.imm = nil
end

-- Fallback for incomplete opcodes at the end.
local function incomplete(ctx)
  ctx.pos = ctx.stop+1
  ctx.o16 = false; ctx.seg = false; ctx.lock = false; ctx.rep = false
  return putop(ctx, "(incomplete)")
end

-- Fallback for unknown opcodes.
local function unknown(ctx)
  ctx.o16 = false; ctx.seg = false; ctx.lock = false; ctx.rep = false
  return putop(ctx, "(unknown)")
end

-- Return an immediate of the specified size.
local function getimm(ctx, pos, n)
  if pos+n-1 > ctx.stop then return incomplete(ctx) end
  local code = ctx.code
  if n == 1 then
    local b1 = byte(code, pos, pos)
    return b1
  elseif n == 2 then
    local b1, b2 = byte(code, pos, pos+1)
    return b1+b2*256
  else
    local b1, b2, b3, b4 = byte(code, pos, pos+3)
    local imm = b1+b2*256+b3*65536+b4*16777216
    ctx.imm = imm
    return imm
  end
end

-- Process pattern string and generate the operands.
local function putpat(ctx, name, pat)
  local operands, regs, sz, mode, sp, rm, sc, rx, disp, sdisp
  local code, pos, stop = ctx.code, ctx.pos, ctx.stop

  -- Chars used: 1DFGMPQRVWXacdfgijmoprsuwxyz
  for p in gmatch(pat, ".") do
    local x = nil
    if p == "V" then
      sz = ctx.o16 and "W" or "D"; ctx.o16 = false
      regs = map_regs[sz]
    elseif match(p, "[BWDQFGMX]") then
      sz = p
      regs = map_regs[sz]
    elseif p == "P" then
      sz = ctx.o16 and "X" or "M"; ctx.o16 = false
      regs = map_regs[sz]
    elseif p == "s" then
      local imm = getimm(ctx, pos, 1); if not imm then return end
      x = imm <= 127 and format("byte +0x%02x", imm)
		     or format("byte -0x%02x", 256-imm)
      pos = pos+1
    elseif p == "u" then
      local imm = getimm(ctx, pos, 1); if not imm then return end
      x = format("0x%02x", imm)
      pos = pos+1
    elseif p == "w" then
      local imm = getimm(ctx, pos, 2); if not imm then return end
      x = format("0x%x", imm)
      pos = pos+2
    elseif p == "o" then -- [offset]
      local imm = getimm(ctx, pos, 4); if not imm then return end
      x = format("[0x%08x]", imm)
      pos = pos+4
    elseif p == "i" then
      local n = map_sz2n[sz]
      local imm = getimm(ctx, pos, n); if not imm then return end
      x = format(imm > 65535 and "0x%08x" or "0x%x", imm)
      pos = pos+n
    elseif p == "j" then
      local n = map_sz2n[sz]
      local imm = getimm(ctx, pos, n); if not imm then return end
      if sz == "B" and imm > 127 then imm = imm-256
      elseif imm > 2147483647 then imm = imm-4294967296 end
      pos = pos+n
      imm = imm + pos + ctx.addr
      ctx.imm = imm
      x = sz == "W" and format("word 0x%04x", imm%65536)
		    or format("0x%08x", imm)
    elseif p == "R" then x = regs[byte(code, pos-1, pos-1)%8+1]
    elseif p == "a" then x = regs[1]
    elseif p == "c" then x = "cl"
    elseif p == "d" then x = "dx"
    elseif p == "1" then x = "1"
    else
      if not mode then
	mode = ctx.mrm
	if not mode then
	  if pos > stop then return incomplete(ctx) end
	  mode = byte(code, pos, pos)
	  pos = pos+1
	end
	rm = mode%8; mode = (mode-rm)/8
	sp = mode%8; mode = (mode-sp)/8
	sdisp = ""
	if mode < 3 then
	  if rm == 4 then
	    if pos > stop then return incomplete(ctx) end
	    sc = byte(code, pos, pos)
	    pos = pos+1
	    rm = sc%8; sc = (sc-rm)/8
	    rx = sc%8; sc = (sc-rx)/8
	    if rx == 4 then rx = nil end
	  end
	  if mode > 0 or rm == 5 then
	    local dsz = mode
	    if dsz ~= 1 then dsz = 4 end
	    disp = getimm(ctx, pos, dsz); if not disp then return end
	    sdisp = (dsz == 4 or disp <= 127) and
		    format(disp > 65535 and "+0x%08x" or "+0x%x", disp) or
		    format("-0x%x", 256-disp)
	    pos = pos+dsz
	  end
	end
      end
      if p == "m" then
	if mode == 3 then x = regs[rm+1]
	else
	  local srm, srx = map_aregs[rm+1], ""
	  if rx then
	    srm = srm.."+"
	    srx = map_aregs[rx+1]
	    if sc > 0 then srx = srx.."*"..(2^sc) end
	  end
	  if mode == 0 and rm == 5 then
	    srm = ""
	    sdisp = format("%s0x%08x", rx and "+" or "", disp)
	  end
	  x = format("[%s%s%s]", srm, srx, sdisp)
	end
	if mode < 3 and
	   (not match(pat, "[aRrgp]") or
	    name == "movzx" or name == "movsx") then -- Yuck.
	  x = map_sz2prefix[sz].." "..x
	end
      elseif p == "r" then x = regs[sp+1]
      elseif p == "g" then x = map_segregs[sp+1]
      elseif p == "p" then -- Suppress prefix.
      elseif p == "f" then x = "st"..rm
      elseif p == "x" then x = "CR"..sp
      elseif p == "y" then x = "DR"..sp
      elseif p == "z" then x = "TR"..sp
      else
	error("bad pattern `"..pat.."'")
      end
    end
    if x then operands = operands and operands..","..x or x end
  end
  ctx.pos = pos
  return putop(ctx, name, operands)
end

-- Forward declaration.
local map_act

-- Get a pattern from an opcode map and dispatch to handler.
local function opcdispatch(ctx, opcmap)
  local pos = ctx.pos
  local opat = opcmap[byte(ctx.code, pos, pos)]
  if not opat then return unknown(ctx) end
  if match(opat, "%|") then -- MMX/SSE variants depending on prefix.
    local p
    if ctx.rep then p = ctx.rep=="rep" and "%|([^%|]*)" or "%|.-%|.-%|([^%|]*)"
    elseif ctx.o16 then p = "%|.-%|([^%|]*)"
    else p = "^[^%|]*" end
    opat = match(opat, p)
    if not opat or opat == "" then return unknown(ctx) end
    ctx.rep = false; ctx.o16 = false
  end
  local name, pat, act = match(opat, "^([a-z0-9 ]*)((.?).*)")
  ctx.pos = pos + 1
  return map_act[act](ctx, name, pat)
end

-- Map for action codes. The key is the first char after the name.
map_act = {
  -- Simple opcodes without operands.
  [""] = function(ctx, name, pat)
    return putop(ctx, name)
  end,

  -- Operand size chars fall right through.
  B = putpat, W = putpat, D = putpat, V = putpat,
  F = putpat, G = putpat,
  M = putpat, X = putpat, P = putpat,

  -- Collect prefixes.
  [":"] = function(ctx, name, pat)
    ctx[pat == ":" and name or sub(pat, 2)] = name
  end,

  -- Select alternate opcode name when prefixed with o16.
  ["/"] = function(ctx, name, pat)
    local wname, rpat = match(pat, "^/([a-z0-9 ]+)(.*)")
    if ctx.o16 then name = wname; ctx.o16 = false end
    return putpat(ctx, name, rpat)
  end,

  -- Chain to special handler specified by name.
  ["*"] = function(ctx, name, pat)
    return map_act[name](ctx, name, sub(pat, 2))
  end,

  -- Use named subtable for opcode group.
  ["!"] = function(ctx, name, pat)

    local pos = ctx.pos
    if pos > ctx.stop then return incomplete(ctx) end
    local mrm = byte(ctx.code, pos, pos)
    ctx.pos = pos+1
    ctx.mrm = mrm

    local opat = map_opcgroup[name][((mrm-(mrm%8))/8)%8+1]
    if not opat then return unknown(ctx) end
    local name, pat2 = match(opat, "^([a-z0-9 ]*)(.*)")
    return putpat(ctx, name, pat2 ~= "" and pat2 or sub(pat, 2))
  end,

  -- Two-byte opcode dispatch.
  opc2 = function(ctx, name, pat)
    return opcdispatch(ctx, map_opc2)
  end,

  -- SSSE3 dispatch.
  ssse3 = function(ctx, name, pat)
    return opcdispatch(ctx, map_ssse3[pat])
  end,

  -- Floating point opcode dispatch.
  fp = function(ctx, name, pat)

    local pos = ctx.pos
    if pos > ctx.stop then return incomplete(ctx) end
    local mrm = byte(ctx.code, pos, pos)
    ctx.pos = pos+1
    ctx.mrm = mrm

    local rm = mrm%8
    local idx = pat*8 + ((mrm-rm)/8)%8
    if mrm >= 192 then idx = idx + 64 end
    local opat = map_opcfp[idx]
    if type(opat) == "table" then opat = opat[rm+1] end
    if not opat then return unknown(ctx) end
    local name, pat2 = match(opat, "^([a-z0-9 ]*)(.*)")
    return putpat(ctx, name, pat2)
  end,
}

------------------------------------------------------------------------------

-- Disassemble a block of code.
local function disass_block(ctx, ofs, len)
  if not ofs then ofs = 0 end
  local stop = len and ofs+len or #ctx.code
  ofs = ofs + 1
  ctx.start = ofs
  ctx.pos = ofs
  ctx.stop = stop
  ctx.imm = nil
  ctx.mrm = false
  ctx.o16 = false; ctx.seg = false; ctx.lock = false; ctx.rep = false
  while ctx.pos <= stop do opcdispatch(ctx, map_opc1) end
  if ctx.pos ~= ctx.start then incomplete(ctx) end
end

-- Extended API: create a disassembler context. Then call ctx:disass(ofs, len).
local function create_(code, addr, out)
  local ctx = {}
  ctx.code = code
  ctx.addr = (addr or 0) - 1
  ctx.out = out or io.write
  ctx.symtab = {}
  ctx.disass = disass_block
  return ctx
end

-- Simple API: disassemble code (a string) at address and output via out.
local function disass_(code, addr, out)
  create_(code, addr, out):disass()
end


-- Public module functions.
module(...)

create = create_
disass = disass_

