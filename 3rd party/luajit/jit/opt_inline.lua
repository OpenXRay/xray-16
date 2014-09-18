----------------------------------------------------------------------------
-- LuaJIT optimizer add-on module for function inlining.
--
-- Copyright (C) 2005-2008 Mike Pall. All rights reserved.
-- Released under the MIT/X license. See luajit.h for full copyright notice.
----------------------------------------------------------------------------
-- This is a simple framework for C function signature maps.
-- It helps with type propagation and C function inlining.
--
-- This module is automatically loaded with -O2 and above.
-- By default most standard library functions are added.
--
-- TODO: generalize it, e.g. for back propagation (i.e. arg types).
-- TODO: extend it for Lua functions (but need to analyze them before use).
------------------------------------------------------------------------------

-- Cache some library functions and objects.
local jit = require("jit")
assert(jit.version_num == 10104, "LuaJIT core/library version mismatch")
local jutil = require("jit.util")
local type, rawget, next = type, rawget, next
local hints, fhints = jutil.hints, jutil.fhints
local sub, match, gsub = string.sub, string.match, string.gsub

-- Turn compilation off for the whole module. LuaJIT would do that anyway.
jit.off(true, true)

-- Prototypical objects used for type hints.
local TABLE = {}
local CFUNC = collectgarbage -- Pretty sure this is never inlined.


-- Map from C closures to signatures. Cannot use a weak table.
-- Closures must be kept alive because inlining checks against their addrs.
local map_sign = {}

-- For jit.dumphints: get printable name for TYPE hint: "#INLINE(foo.bar)".
local function getname_(f, idx)
  local sign = map_sign[f]
  if sign then
    local libname, name = sign.libname, sign.name
    if libname then
      return libname.."."..name
    else
      return name
    end
  elseif idx == 0 then
    return "recursive"
  else
    return "?"
  end
end

-- Name, base table and running index for convenience functions.
-- CHECK: the library index and the order below must match with ljit_hints.h
local flibname, flib, fidx

local function fadd(name, results, args, handler)
  local f = rawget(flib, name)
  if f then
    map_sign[f] = {
      libname = flibname, name = name, idx = fidx,
      results = results, args = args, handler = handler,
    }
  end
  if fidx then fidx = fidx + 1 end
end

local function faddf(name, f, results, args, handler)
  map_sign[f] = {
    libname = flibname, name = name, idx = fidx,
      results = results, args = args, handler = handler,
  }
  if fidx then fidx = fidx + 1 end
end


-- Signature handler: copy first argument to first result.
local function copyfirst(st, slot, pc, base, narg, nres)
  slot[base] = slot[base+1]
end

-- Helper for iterators: check if the function is an iterator constructor.
--
-- 'for ivars in func(args) do body end'
--
--   ...load func+args...
--   CALL func           <-- pc
--   JMP fwd  ---+
-- back:         | <--+
--   ...body...  |    |
-- fwd:       <--+    |
--   TFORLOOP ivars   |  <-- tforpc
--   JMP back      ---+
--
local function itercheck(st, slot, pc, base, idx)
  if idx then
    local bytecode, func = jutil.bytecode, st.func
    local op, _, fwd = bytecode(func, pc+1)
    if op == "JMP" then
      local tforpc = pc+2+fwd
      local op, tfbase, _, tfnres = bytecode(func, tforpc)
      if op == "TFORLOOP" and tfbase == base and tfnres <= 2 then
	local op, _, back = bytecode(func, tforpc+1)
	if op == "JMP" and fwd+back == -2 then
	  -- Must set inlining hint for TFORLOOP instruction here.
	  st[tforpc+hints.INLINE] = idx -- Serves as iterator index, too.
	  return -- Inline it.
	end
      end
    end
  end
  slot[base] = CFUNC -- Better make it different from pairs.
  return true -- Better not inline it, if not used in a for statement.
end

-- Helper for pairs/next: guess result types for standard table iterator.
local function guessnext(st, slot, base, dest)
  local t, k, v = slot[base+1]
  if type(t) == "table" then
    k, v = next(t)
    if v == nil then v = st.tableval[t] end
  end
  slot[dest] = k or "" -- Strings are a good guess for the key type.
  slot[dest+1] = v -- But better not guess any fixed value type.
end


-- Signatures for base library functions.
-- Note: Only add functions where result type hints or inlining makes sense.
flibname, flib, fidx = nil, _G, 65536*1
fadd("pairs",		"..0", "T",
  function(st, slot, pc, base, narg, nres, idx)
    -- Table in slot[base+1] is kept (2nd result = 1st arg).
    -- Fill result slots for the iterator here (TFORLOOP is at the end).
    guessnext(st, slot, base, base+3)
    return itercheck(st, slot, pc, base, idx)
  end)

fadd("ipairs",	"..I", "T",
  function(st, slot, pc, base, narg, nres, idx)
    -- Table in slot[base+1] is kept (2nd result = 1st arg).
    -- Fill result slots for the iterator here (TFORLOOP is at the end).
    local t = slot[base+1]
    slot[base+3] = 1 -- Integer key.
    local v
    if type(t) == "table" then
      v = rawget(t, 1)
      if v == nil then v = st.tableval[t] end
    end
    slot[base+4] = v
    return itercheck(st, slot, pc, base, idx)
  end)

fidx = nil -- Pure result type signatures follow:
fadd("next",		"..", "T.?",
  function(st, slot, pc, base, narg, nres)
    guessnext(st, slot, base, base)
  end)
fadd("type",		"S", ".")
fadd("getmetatable",	"T", ".")
fadd("setmetatable",	".", "TT?", copyfirst)
fadd("rawequal",	"B", "..")
fadd("rawget",		".", "T.",
  function(st, slot, pc, base, narg, nres)
    local t = slot[base+1]
    slot[base] = type(t) == "table" and rawget(t, slot[base+2]) or ""
  end)
fadd("rawset",		".", "T..", copyfirst)
fadd("assert",		"*", "..*",
  function(st, slot, pc, base, narg, nres)
    for i=1,nres do slot[base+i-1] = i <= narg and slot[base+i] or nil end
  end)
fadd("tonumber",	"I", ".I?")
fadd("tostring",	"S", ".")
fadd("require",		"T", "S")

-- Signatures for coroutine library functions.
flibname, flib, fidx = "coroutine", coroutine, 65536*2
if flib then
  fadd("yield",		"*", ".*")
  fadd("resume",	"*", "R.*",
    function(st, slot, pc, base, narg, nres)
      slot[base] = true
      for i=1,nres-1 do slot[base+i] = nil end -- No guess.
    end)

  fidx = nil -- Pure result type signatures follow:
  fadd("wrap",		"C", "F")
end

-- Signatures for string library functions.
flibname, flib, fidx = "string", string, 65536*3
if flib then
  fadd("len",		"I", "S")
  fadd("sub",		"S", "SII?")
  fadd("char",		"S", "I*")

  fidx = nil -- Pure result type signatures follow:
  fadd("byte",		"I", "S",
    function(st, slot, pc, base, narg, nres)
      for i=0,nres-1 do slot[base+i] = 1 end -- Set all result hints.
    end)
  fadd("rep",		"S", "SI")
  fadd("reverse",	"S", "S")
  fadd("upper",		"S", "S")
  fadd("lower",		"S", "S")

  fadd("format",	"S", "S.*")
  fadd("find",		"*", "SSI?.?",
    function(st, slot, pc, base, narg, nres)
      slot[base] = 1
      slot[base+1] = 1
      for i=2,nres-1 do slot[base+i] = "" end -- Hints for matches.
    end)
  fadd("match",		"*", "SSI?",
    function(st, slot, pc, base, narg, nres)
      for i=0,nres-1 do slot[base+i] = "" end -- Hints for matches.
    end)
  fadd("gsub",		"SI", "SSGI?")
  fadd("gmatch",	"C00", "SS",
    function(st, slot, pc, base, narg, nres)
      -- Fill result slots for gmatch_iter here (TFORLOOP is at the end).
      for i=base+3,st.stats.stackslots-1 do slot[i] = "" end
    end)
  -- The gmatch iterator itself is never inlined. No point in adding it.
end

-- Signatures for table library functions.
flibname, flib, fidx = "table", table, 65536*4
if flib then
  fadd("insert",	"", "TI?.")
  fadd("remove",	".", "T",
    function(st, slot, pc, base, narg, nres)
      if nres >= 1 then
	local t = slot[base+1]
	slot[base] = type(t) == "table" and rawget(t, 1) or ""
      end
    end)
  fadd("getn",		"I", "T")

  fidx = nil -- Pure result type signatures follow:
  fadd("concat",	"S", "TS?I?I?")
end

-- Signatures for math library functions.
flibname, flib, fidx = "math", math, 65536*5
if flib then
  -- 1 arg, 1 result.
  fadd("log",		"N", "N")
  fadd("log10",		"N", "N")
  fadd("exp",		"N", "N")
  fadd("sinh",		"N", "N")
  fadd("cosh",		"N", "N")
  fadd("tanh",		"N", "N")
  fadd("asin",		"N", "N")
  fadd("acos",		"N", "N")
  fadd("atan",		"N", "N")
  fadd("sin",		"N", "N")
  fadd("cos",		"N", "N")
  fadd("tan",		"N", "N")
  fadd("ceil",		"I", "N")
  fadd("floor",		"I", "N")
  fadd("abs",		".", "N", copyfirst)
  fadd("sqrt",		"N", "N")
  -- 2 args, 1 result.
  -- math.fmod is aliased to math.mod for compatibility.
  fadd("fmod",		".", "NN",
    function(st, slot, pc, base, narg, nres)
      slot[base] = slot[base+2] or 1 -- Copy integer or number hint.
    end)
  fadd("atan2",		"N", "NN")

  fidx = nil -- Pure result type signatures follow:
  -- 1-n args, 1 result.
  fadd("min",		".", "NN*", copyfirst) -- Really depends on all args.
  fadd("max",		".", "NN*", copyfirst) -- Really depends on all args.
  -- 1 arg, 1 result.
  fadd("deg",		"N", "N")
  fadd("rad",		"N", "N")
  -- 1 arg, 2 results.
  fadd("modf",		"IN", "N")
  fadd("frexp",		"NI", "N")
  -- 2 args, 1 result.
  fadd("pow",		"N", "NN")
  fadd("ldexp",		".", "NI", copyfirst)
  -- 1 arg, 0 results.
  fadd("randomseed",	"", "I")
  -- 0-2 args, 1 result.
  fadd("random",	"N", "I?I?",
    function(st, slot, pc, base, narg, nres)
      if narg > 0 then slot[base] = 1 end
    end)
end

-- Signatures for I/O library functions.
-- Not inlined anytime soon. Used for result types only.
flibname, flib, fidx = "io", io, nil
if flib then
  fadd("lines",		"C00S", "S?")
  fadd("read",		"S", "") -- Simplified (a lot).
  -- Adding io methods doesn't work, because we don't deal with userdata (yet).
end


-- Type names to argument type shorthands.
-- TODO: make the matches more exact? Need to differentiate nil/unknown.
local map_argtype = {
  ["nil"] = "0", boolean = "b", number = "n", string = "s",
  table = "t", ["function"] = "f", userdata = "u", thread = "r",
}

-- Complex argument match patterns to regexp fragments.
local map_argmatch = {
  B = "[b0]", S = "[s0]", T = "[t0]", F = "[f0]", U = "[u0]", R = "[r0]",
  N = "[n0]", I = "[n0]", -- Number/int args are the same for now.
  G = "[stf0]", -- For string.gsub.
}

-- Result type shorthands to sample types.
local map_restype = {
  -- ["0"] = nil,
  B = true, S = "", T = {},
  N = 0.5, I = 1,
  L = function() end, C = collectgarbage, -- Pretty sure this is never inlined.
}

-- Create argument match regexp and cache it.
local function getargmatch(sign)
  local argmatch = "^"..gsub(sign.args, ".", map_argmatch).."0*$"
  sign.argmatch = argmatch
  return argmatch
end

-- Set INLINE hints and result types for known C functions.
local function inlinehint(sign, st, slot, pc, base, narg, nres)
  local idx = sign.idx
  if idx then
    if narg ~= -1 then
      local argpat = ""
      for i=1,narg do argpat = argpat..map_argtype[type(slot[base+i])] end
      if not match(argpat, sign.argmatch or getargmatch(sign)) then
	idx = nil
      end
    end
  end

  local results = sign.results
  if results ~= "*" and nres ~= -1 then
    if nres > #results then idx = nil end
    for i=1,#results do
      local c = sub(results, i, i)
      if c ~= "." then slot[base+i-1] = map_restype[c] end
    end
  end

  local handler = sign.handler
  if handler and handler(st, slot, pc, base, narg, nres, idx) then idx = nil end

  if idx then st[pc+hints.INLINE] = idx end
end

-- Set call hints and result types during forward propagation.
local function fwdcallhint(st, slot, pc, base, narg, nres)
  local f = slot[base]
  st[pc+hints.TYPE] = f
  if type(f) == "function" then
    local sign = map_sign[f]
    if sign then
      inlinehint(sign, st, slot, pc, base, narg, nres)
      return
    end
    if f == st.func and not st.stats.isvararg and
       (narg == -1 or narg == st.stats.params) then
      st[pc+hints.INLINE] = 0 -- Recursive call.
    end
  end
  -- Clear result types for unknown functions.
  for i=base,base+nres-1 do slot[i] = nil end
end


-- Attach call hinter to optimizer.
local function start_()
  local jopt = require "jit.opt"
  jopt.attach_callhint(fwdcallhint)
  -- Note that just loading the optimizer does not start it, yet.
end


-- Public module functions.
module(...)

-- TODO: Public API to add signatures. Alas, the API is still in flux.
getname = getname_
start = start_

