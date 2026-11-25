-- PW Edition: conservative session state for connected standalone areas.
-- Unlike the built-in weapon slots, these are user-defined scalar quest/item flags.
-- Persisted only when GameLoopSaveStats/GameLoopLoadStats are called by the engine.
-- No executable Lua is ever loaded from this file.
local PW = {}
local values = {}
local path = "savegames\\pw_transition_state.dat"
local MAX_KEYS = 128
local MAX_KEY = 64
local MAX_VALUE_BYTES = 512

local function goodkey(k)
 return type(k) == "string" and #k >= 1 and #k <= MAX_KEY
    and k:match("^[%w_.%-]+$") ~= nil
end

local function goodvalue(v)
 local t = type(v)
 if t == "string" then return #v <= MAX_VALUE_BYTES end
 if t == "boolean" then return true end
 if t == "number" then return v == v and v ~= math.huge and v ~= -math.huge end
 return false
end

function PW.set(key, value)
 if not goodkey(key) then return false end
 if value == nil then values[key] = nil return true end
 if not goodvalue(value) then return false end
 if values[key] == nil then
  local total = 0
  for _ in pairs(values) do total = total + 1 end
  if total >= MAX_KEYS then return false end
 end
 values[key] = value
 return true
end

function PW.get(key, fallback)
 if not goodkey(key) then return fallback end
 local stored = values[key]
 if stored == nil then return fallback end
 return stored
end

function PW.clear()
 values = {}
end

local function encode(value)
 local valueType = type(value)
 local kind = valueType == "boolean" and "b"
           or valueType == "number" and "n" or "s"
 local plain = valueType == "boolean" and (value and "1" or "0") or tostring(value)
 local hex = {}
 for index = 1, #plain do
  hex[index] = string.format("%02X", string.byte(plain, index))
 end
 return kind, table.concat(hex)
end

local function decode(kind, hex)
 if #hex % 2 ~= 0 or #hex > MAX_VALUE_BYTES * 2 then return nil, false end
 local bytes = {}
 for i = 1, #hex, 2 do
  local n = tonumber(hex:sub(i, i + 1), 16)
  if not n then return nil, false end
  bytes[#bytes + 1] = string.char(n)
 end
 local plain = table.concat(bytes)
 if kind == "s" then return plain, true end
 if kind == "b" then
  if plain == "1" then return true, true end
  if plain == "0" then return false, true end
  return nil, false
 end
 if kind == "n" then
  local number = tonumber(plain)
  if goodvalue(number) then return number, true end
 end
 return nil, false
end

function PW.export()
 local names = {}
 for key, value in pairs(values) do
  if not goodkey(key) or not goodvalue(value) then return false end
  names[#names + 1] = key
 end
 if #names > MAX_KEYS then return false end
 table.sort(names)
 local lines = {"PWSTATE1"}
 for _, key in ipairs(names) do
  local kind, hex = encode(values[key])
  lines[#lines+1] = key .. "|" .. kind .. "|" .. hex
 end
 lines[#lines+1] = "END"
 return table.concat(lines,"\n") .. "\n"
end

function PW.parse(data)
 if type(data) ~= "string" or #data > 150000 or data:sub(-1) ~= "\n" then return nil end
 local restored, count, first, complete = {}, 0, true, false
 for line in data:gmatch("([^\n]*)\n") do
  if first then
   if line ~= "PWSTATE1" then return nil end
   first = false
  elseif complete then return nil
  elseif line == "END" then complete = true
  else
   if count >= MAX_KEYS or #line > MAX_KEY + MAX_VALUE_BYTES*2+4 then return nil end
   local key, kind, hex = line:match("^([%w_.%-]+)|([snb])|([%x]*)$")
   if not goodkey(key) or restored[key] ~= nil then return nil end
   local value, valid = decode(kind, hex)
   if not valid then return nil end
   restored[key] = value
   count = count + 1
  end
 end
 if not complete then return nil end
 return restored
end
function PW.import(data)
 local restored = PW.parse(data)
 if not restored then return false end
 values = restored
 return true
end
function PW.save()
 local data = PW.export()
 if not data then return false end
 local file = io.open(path,"wb")
 if not file then return false end
 local ok=file:write(data)
 local closed=file:close()
 return ok ~= nil and closed ~= nil
end
function PW.load()
 local file=io.open(path,"rb")
 if not file then return false end
 local data=file:read(150001)
 file:close()
 return PW.import(data)
end

return PW
