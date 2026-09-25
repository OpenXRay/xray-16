assert(assert(loadstring("return 6 /* fixture */ / 2 // fixture\n"))() == 3)
assert(coroutine.cstacksize(4096) == 4096)
assert(coroutine.cstacksize() == -1)
assert(not pcall(raise_error))
assert(select(2, loadstring("OK")):match("OK"))

local ffi = require("ffi")
ffi.cdef[[int snprintf(char *str, size_t size, const char *format, ...);]]
local output = ffi.new("char[128]")
assert(ffi.C.snprintf(output, 128, "%d %d %d %d %d %d %d %d %d %d",
    ffi.new("int", 1), ffi.new("int", 2), ffi.new("int", 3), ffi.new("int", 4),
    ffi.new("int", 5), ffi.new("int", 6), ffi.new("int", 7), ffi.new("int", 8),
    ffi.new("int", 9), ffi.new("int", 10)) == 20)
assert(ffi.string(output) == "1 2 3 4 5 6 7 8 9 10")

local buffer = require("string.buffer")
local value = buffer.decode(buffer.encode({number = 42, text = "hello", list = {1, 2, 3}}))
assert(value.number == 42 and value.text == "hello" and value.list[3] == 3)

local names = {}
for i = 1, 20000 do
    names["key" .. i] = "value" .. i
end
collectgarbage("collect")
for i = 1, 20000 do
    assert(names["key" .. i] == "value" .. i)
end

local finalized = 0
for i = 1, 100 do
    local object = newproxy(true)
    getmetatable(object).__gc = function() finalized = finalized + 1 end
end
collectgarbage("collect")
collectgarbage("collect")
assert(finalized == 100)

jit.flush()
jit.opt.start("hotloop=1")
local function sum(n)
    local total = 0
    for i = 1, n do total = total + i end
    return total
end
assert(sum(10000) == 50005000)
assert(sum(10000) == 50005000)
assert(require("jit.util").traceinfo(1))
print(jit.version .. ": integration passed")
