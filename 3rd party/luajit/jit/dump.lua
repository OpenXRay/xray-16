----------------------------------------------------------------------------
-- LuaJIT machine code dumper module.
--
-- Copyright (C) 2005-2008 Mike Pall. All rights reserved.
-- Released under the MIT/X license. See luajit.h for full copyright notice.
----------------------------------------------------------------------------
-- Activate this module to dump the machine code for all functions
-- immediately after they have been compiled. The disassembler
-- output is mixed with the bytecode listing.
--
-- Try: luajit -j dump -e 'print "foo"'
--      luajit -j dump=foo.dump foo.lua
--      luajit -j off -j dump -e 'jit.compile(assert(loadfile"foo.lua")))'
--
-- Default output is to stderr. To redirect output to a file,
-- pass a filename as an argument or set the environment variable
-- "LUAJIT_DUMPFILE".
-- Note: The file is overwritten each time you run luajit.
--
-- TODO: Find a way to be more selective on what to dump.
------------------------------------------------------------------------------

-- Priority for compiler pipeline. Must run after backend (negative)
-- and should be even because we only catch successful compiles.
local PRIORITY = -98

-- Cache some library functions and objects.
local jit = require("jit")
assert(jit.version_num == 10104, "LuaJIT core/library version mismatch")
local jutil = require("jit.util")
local type, format, gsub = type, string.format, string.gsub
local bytecode, const = jutil.bytecode, jutil.const
local getinfo = debug.getinfo
local stdout, stderr = io.stdout, io.stderr

-- Load the right disassembler.
local dis = require("jit.dis_"..jit.arch)
local discreate, disass_ = dis.create, dis.disass

-- Turn compilation off for the whole module. LuaJIT would do that anyway.
jit.off(true, true)

-- Separator line.
local sepline = "-------------------------------"

-- Map JSUB indices to names.
-- CHECK: must match the order in ljit_x86.h. Regenerate with:
--   grep '^ *JSUB_[^_].*,' ljit_x86.h | sed -e 's/^ *JSUB_/  "/' -e 's/,.*/",/'
local jsubnames = {
  "STACKPTR",
  "GATE_LJ",
  "GATE_JL",
  "GATE_JC",
  "GROW_STACK",
  "GROW_CI",
  "GATE_JC_PATCH",
  "GATE_JC_DEBUG",
  "DEOPTIMIZE_CALLER",
  "DEOPTIMIZE",
  "DEOPTIMIZE_OPEN",
  "HOOKINS",
  "GCSTEP",
  "STRING_SUB3",
  "STRING_SUB2",
  "HOOKCALL",
  "HOOKRET",
  "METACALL",
  "METATAILCALL",
  "BARRIERF",
  "GETGLOBAL",
  "GETTABLE_KSTR",
  "GETTABLE_STR",
  "BARRIERBACK",
  "SETGLOBAL",
  "SETTABLE_KSTR",
  "SETTABLE_STR",
  "GETTABLE_KNUM",
  "GETTABLE_NUM",
  "SETTABLE_KNUM",
  "SETTABLE_NUM",
  "LOG2_TWORD",
  "CONCAT_STR2",
}

-- Generate map from JSUB addresses to JSUB names.
local jsubmap = {}
do
  local jsubmcode = jutil.jsubmcode
  for pc=0,100000 do
    local addr = jsubmcode(pc)
    if not addr then break end
    jsubmap[addr] = jsubnames[pc+1] or "JSUB#"..pc
  end
end

-- Pretty-print a constant.
local function conststr(func, idx)
  local k = const(func, idx)
  if k == nil then return "nil"
  elseif k == true then return "true"
  elseif k == false then return "false"
  elseif type(k) == "string" then
    if #k > 10 then return format('"%.10s"~', k)
    else return '"'..k..'"' end
  else return k.."" end
end

-- Pretty-print a bytecode instruction (one or two lines).
local function bytecodeout(out, func, pc)
  local op, a, b, c = bytecode(func, pc)
  if not op then
    return true
  elseif op == "JMP" then
    out:write(format("\n--%04d--  JMP   => %04d", pc, pc+1+b))
  elseif op == "FORLOOP" or op == "FORPREP" then
    out:write(format("\n--%04d--  %-9s %3d => %04d", pc, op, a, pc+1+b))
  else
    out:write(format("\n--%04d--  %-9s %3d %4s %4s",
		     pc, op, a, b or "", c or ""))
    if b and b < 0 then out:write(" ; ", conststr(func, b)) end
    if c and c < 0 then out:write(" ; ", conststr(func, c)) end
  end
end

-- Dump machine code and mix it with the bytecode listing.
local function dumpfunc(func, out, deopt)
  if not out then out = stderr end
  local info = getinfo(func, "S")

  -- Don't bother checking for the right blocks to dump.
  -- Dump the main block (if not deopt) and always all deopt blocks.
  for block=deopt and 2 or 1,1000000 do
    local addr, code, mfmiter = jutil.mcode(func, block)
    if not addr then
      if code then return code end -- Not compiled: return status.
      break -- No more blocks to dump.
    end

    -- Print header.
    out:write(sepline, " ", info.source, ":", info.linedefined)
    if block ~= 1 then out:write("  DEOPT block ", block) end

    -- Create disassembler context.
    local ctx = discreate(code, addr, function(s) out:write(s) end)
    ctx.symtab = jsubmap

    -- Dump an mcode block.
    local pc, ofs = 1, 0
    local len, isdeopt = mfmiter()
    if isdeopt then pc = len; len = 0
    elseif block ~= 1 then break end -- Stop before next main block.
    for t, m in mfmiter do
      if t == "COMBINE" then
	bytecodeout(out, func, pc)
      else
	if len ~= 0 then
	  out:write("\n")
	  if len > 0 then
	    ctx:disass(ofs, len)
	    ofs = ofs + len
	  else
	    out:write(format("%08x  ** deoptimized\n", addr+ofs))
	    ofs = ofs - len
	  end
	  len = 0
	end
	if type(t) == "number" then
	  if m then
	    if isdeopt then
	      pc = t - 1
	    else
	      bytecodeout(out, func, pc)
	      len = -t
	    end
	  else
	    len = t
	    if bytecodeout(out, func, pc) then break end
	  end
	end
      end
      pc = pc + 1
    end
    if len and len ~= 0 then
      out:write(sepline, " tail code\n")
      ctx:disass(ofs, len)
    end
  end

  -- Print footer.
  out:write(sepline, "\n")
  out:flush()
end

-- Dump the internal JIT subroutines.
local function dumpjsub_(out)
  if not out then out = stderr end
  local addr, code = jutil.jsubmcode()

  -- Create disassembler context.
  local ctx = discreate(code, addr, function(s) out:write(s) end)
  ctx.symtab = jsubmap

  -- Collect addresses and sort them.
  local t = {}
  for addr in pairs(jsubmap) do t[#t+1] = addr end
  t[#t+1] = addr + #code
  table.sort(t)

  -- Go through the addresses in ascending order.
  local ofs = addr
  for i=2,#t do
    local next = t[i]
    out:write("\n->", jsubmap[ofs], ":\n") -- Output label for JSUB.
    ctx:disass(ofs-addr, next-ofs) -- Disassemble corresponding code block.
    ofs = next
  end
  out:flush()
end


-- Active flag and output file handle.
local active, out

-- Dump handler for compiler pipeline.
local function h_dump(st)
  local ok, err = pcall(dumpfunc, st.func, out, st.deopt)
  if not ok then
    stderr:write("\nERROR: jit.dump disabled: ", err, "\n")
    jit.attach(h_dump) -- Better turn ourselves off after a failure.
    if out and out ~= stdout then out:close() end
    out = nil
    active = nil
  end
end

-- Detach dump handler from compiler pipeline.
local function dumpoff()
  if active then
    active = false
    jit.attach(h_dump)
    if out and out ~= stdout then out:close() end
    out = nil
  end
end

-- Open the output file and attach dump handler to compiler pipeline.
local function dumpon(filename)
  if active then dumpoff() end
  local outfile = filename or os.getenv("LUAJIT_DUMPFILE")
  out = outfile and (outfile == "-" and stdout or assert(io.open(outfile, "w")))
  jit.attach(h_dump, PRIORITY)
  active = true
end


-- Public module functions.
module(...)

disass = disass_
dump = dumpfunc
dumpjsub = dumpjsub_
on = dumpon
off = dumpoff
start = dumpon -- For -j command line option.

