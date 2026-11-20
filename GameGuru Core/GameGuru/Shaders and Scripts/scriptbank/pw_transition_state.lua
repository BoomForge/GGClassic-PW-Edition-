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

function PW.save()
 local names = {}
 for key, value in pairs(values) do
  if not goodkey(key) or not goodvalue(value) then return false end
  names[#names + 1] = key
 end
 if #names > MAX_KEYS then return false end
 table.sort(names)
 local file = io.open(path, "wb")
 if not file then return false end
 local ok = file:write("PWSTATE1\n")
 if ok then
  for _, key in ipairs(names) do
   local kind, hex = encode(values[key])
   ok = file:write(key, "|", kind, "|", hex, "\n")
   if not ok then break end
  end
 end
 if ok then ok = file:write("END\n") end
 local closed = file:close()
 return ok ~= nil and closed ~= nil
end

function PW.load()
 local file = io.open(path, "rb")
 if not file then return false end
 if file:read("*l") ~= "PWSTATE1" then file:close() return false end
 local restored = {}
 local count = 0
 local complete = false
 while count <= MAX_KEYS do
  local line = file:read("*l")
  if not line then break end
  if line == "END" then
   complete = file:read("*l") == nil
   break
  end
  if #line > MAX_KEY + MAX_VALUE_BYTES * 2 + 4 then break end
  local key, kind, hex = line:match("^([%w_.%-]+)|([snb])|([%x]*)$")
  if not goodkey(key) or restored[key] ~= nil then break end
  local value, valid = decode(kind, hex)
  if not valid then break end
  restored[key] = value
  count = count + 1
 end
 file:close()
 if not complete then return false end -- never apply partially written/corrupt data
 values = restored
 return true
end

return PW
