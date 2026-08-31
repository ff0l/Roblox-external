if not hookmetamethod then
    return "hookmetamethod required"
end

local _plrs    = game:GetService("Players")
local _rs      = game:GetService("ReplicatedStorage")
local _http    = game:GetService("HttpService")
local _lp      = _plrs.LocalPlayer
local _pscripts = _lp:WaitForChild("PlayerScripts", 15)
local _ctrl    = _pscripts:WaitForChild("Controllers", 15)
local _mods    = _rs:WaitForChild("Modules", 15)

if not _ctrl or not _mods then
    return "Rivals modules not ready"
end

local _enumLib = require(_mods:WaitForChild("EnumLibrary", 10))
if _enumLib then pcall(function() _enumLib:WaitForEnumBuilder() end) end

local _cosLib  = require(_mods:WaitForChild("CosmeticLibrary", 10))
require(_mods:WaitForChild("ItemLibrary", 10))
local _datCtrl = require(_ctrl:WaitForChild("PlayerDataController", 10))

local _eq, _favs = {}, {}
local _buildingWep = nil

local function _mkCosmetic(nm, ctype, opts)
    local _base = _cosLib.Cosmetics[nm]
    if not _base then return nil end
    local _d = {}
    for k, v in pairs(_base) do _d[k] = v end
    _d.Name = nm
    _d.Type = _d.Type or ctype
    _d.Seed = _d.Seed or math.random(1, 1000000)
    if _enumLib then
        local _s, _eid = pcall(_enumLib.ToEnum, _enumLib, nm)
        if _s and _eid then
            _d.Enum = _eid
            _d.ObjectID = _d.ObjectID or _eid
        end
    end
    if opts then
        if opts.inverted ~= nil then _d.Inverted = opts.inverted end
        if opts.favoritesOnly ~= nil then _d.OnlyUseFavorites = opts.favoritesOnly end
    end
    return _d
end

local _cfgFile = "rivals_unlocker_config.json"
local _saveLock = false

local function _stripForSave()
    local _out = {}
    for wn, cos in pairs(_eq) do
        _out[wn] = {}
        for ct, cd in pairs(cos) do
            if cd and cd.Name then
                _out[wn][ct] = {
                    Name = cd.Name,
                    Inverted = cd.Inverted,
                    OnlyUseFavorites = cd.OnlyUseFavorites
                }
            end
        end
    end
    return { equipped = _out, favorites = _favs }
end

local function _loadCfg()
    if not isfile or not readfile then return end
    local _ok1, _ex = pcall(isfile, _cfgFile)
    if not _ok1 or not _ex then return end
    local _ok2, _raw = pcall(readfile, _cfgFile)
    if not _ok2 or not _raw or _raw == "" then return end
    local _ok3, _dec = pcall(_http.JSONDecode, _http, _raw)
    if not _ok3 or not _dec then return end
    if _dec.favorites then
        _favs = _dec.favorites
    end
    if _dec.equipped then
        _eq = {}
        for wn, cos in pairs(_dec.equipped) do
            _eq[wn] = {}
            for ct, sd in pairs(cos) do
                if sd and sd.Name and _cosLib.Cosmetics[sd.Name] then
                    local _cloned = _mkCosmetic(sd.Name, ct, {
                        inverted = sd.Inverted,
                        favoritesOnly = sd.OnlyUseFavorites
                    })
                    if _cloned then
                        _eq[wn][ct] = _cloned
                    end
                end
            end
            if not next(_eq[wn]) then _eq[wn] = nil end
        end
    end
end

local function _saveCfg()
    if not writefile or _saveLock then return end
    _saveLock = true
    task.spawn(function()
        task.wait(1)
        local _payload = _stripForSave()
        local _ok, _enc = pcall(_http.JSONEncode, _http, _payload)
        if _ok then
            pcall(writefile, _cfgFile, _enc)
        end
        _saveLock = false
    end)
end

_loadCfg()

local _cosTypes = {"Skin","Wrap","Charm","Dance","Emote"}
local function _isCosType(cosObj)
    if not cosObj then return false end
    for _, t in ipairs(_cosTypes) do
        if cosObj.Type == t then return true end
    end
    return false
end

_cosLib.OwnsCosmeticNormally = function(self, inv, nm, wep)
    local c = _cosLib.Cosmetics[nm]
    if c and c.Type == "Skin" then return true end
    return false
end
_cosLib.OwnsCosmeticUniversally = function(self, inv, nm, wep)
    local c = _cosLib.Cosmetics[nm]
    if c and c.Type == "Skin" then return true end
    return false
end
_cosLib.OwnsCosmeticForWeapon = function(self, inv, nm, wep)
    local c = _cosLib.Cosmetics[nm]
    if c and c.Type == "Skin" then return true end
    return false
end

local _origOwns = _cosLib.OwnsCosmetic
_cosLib.OwnsCosmetic = function(self, inv, nm, wep)
    if nm:find("MISSING_") or nm == "Bubble Gun" then
        return _origOwns(self, inv, nm, wep)
    end
    local c = _cosLib.Cosmetics[nm]
    if c and _isCosType(c) then return true end
    return _origOwns(self, inv, nm, wep)
end

local _origGet = _datCtrl.Get
_datCtrl.Get = function(self, key)
    local _val = _origGet(self, key)
    if key == "CosmeticInventory" then
        local _prx = {}
        if _val then
            for k, v in pairs(_val) do
                local c = _cosLib.Cosmetics[k]
                if c and _isCosType(c) then _prx[k] = v end
            end
        end
        return setmetatable(_prx, {
            __index = function(t, k)
                local c = _cosLib.Cosmetics[k]
                if c and _isCosType(c) then return true end
                return nil
            end
        })
    end
    if key == "FavoritedCosmetics" then
        local _res = _val and table.clone(_val) or {}
        for wep, fv in pairs(_favs) do
            _res[wep] = _res[wep] or {}
            for nm, isFav in pairs(fv) do
                local c = _cosLib.Cosmetics[nm]
                if c and _isCosType(c) then
                    _res[wep][nm] = isFav
                end
            end
        end
        return _res
    end
    return _val
end

local _origGetWep = _datCtrl.GetWeaponData
_datCtrl.GetWeaponData = function(self, wn)
    local _d = _origGetWep(self, wn)
    if not _d then return nil end
    local _m = {}
    for k, v in pairs(_d) do _m[k] = v end
    _m.Name = wn
    if _eq[wn] then
        for ct, cd in pairs(_eq[wn]) do
            _m[ct] = cd
        end
    end
    return _m
end

local _fightCtrl
pcall(function()
    _fightCtrl = require(_ctrl:WaitForChild("FighterController", 10))
end)

local _remotes   = _rs:FindFirstChild("Remotes")
local _dataRem   = _remotes and _remotes:FindFirstChild("Data")
local _equipRem  = _dataRem and _dataRem:FindFirstChild("EquipCosmetic")
local _favRem    = _dataRem and _dataRem:FindFirstChild("FavoriteCosmetic")
local _repRem    = _remotes and _remotes:FindFirstChild("Replication")
local _fightRem  = _repRem and _repRem:FindFirstChild("Fighter")
local _useItmRem = _fightRem and _fightRem:FindFirstChild("UseItem")

if _equipRem then
    local _onc
    _onc = hookmetamethod(game, "__namecall", function(self, ...)
        if getnamecallmethod() ~= "FireServer" then
            return _onc(self, ...)
        end
        local _a = {...}

        if _useItmRem and self == _useItmRem and _fightCtrl then
            pcall(function()
                local _f = _fightCtrl:GetFighter(_lp)
                if _f and _f.Items then
                    for _, itm in pairs(_f.Items) do
                        if itm:Get("ObjectID") == _a[1] then
                            _buildingWep = itm.Name
                            break
                        end
                    end
                end
            end)
        end

        if self == _equipRem then
            local _wn   = _a[1]
            local _ct   = _a[2]
            local _cn   = _a[3]
            local _opts = _a[4] or {}
            if _cn and _cn ~= "None" and _cn ~= "" then
                local _inv = _datCtrl:Get("CosmeticInventory")
                if _inv and rawget(_inv, _cn) then
                    return _onc(self, ...)
                end
            end
            _eq[_wn] = _eq[_wn] or {}
            if not _cn or _cn == "None" or _cn == "" then
                _eq[_wn][_ct] = nil
                if not next(_eq[_wn]) then _eq[_wn] = nil end
            else
                local _cloned = _mkCosmetic(_cn, _ct, {
                    inverted = _opts.IsInverted,
                    favoritesOnly = _opts.OnlyUseFavorites
                })
                if _cloned then _eq[_wn][_ct] = _cloned end
            end
            task.defer(function()
                pcall(function() _datCtrl.CurrentData:Replicate("WeaponInventory") end)
            end)
            _saveCfg()
            return
        end

        if self == _favRem then
            local _cos = _cosLib.Cosmetics[_a[2]]
            if _cos then
                _favs[_a[1]] = _favs[_a[1]] or {}
                _favs[_a[1]][_a[2]] = _a[3] or nil
                task.spawn(function()
                    pcall(function() _datCtrl.CurrentData:Replicate("FavoritedCosmetics") end)
                end)
                _saveCfg()
            end
            return
        end

        return _onc(self, ...)
    end)
end

local _cliItem
pcall(function()
    _cliItem = require(_lp.PlayerScripts.Modules.ClientReplicatedClasses.ClientFighter.ClientItem)
end)

if _cliItem and _cliItem._CreateViewModel then
    local _origCVM = _cliItem._CreateViewModel
    _cliItem._CreateViewModel = function(self, vmRef)
        local _wn  = self.Name
        local _wp  = self.ClientFighter and self.ClientFighter.Player
        _buildingWep = (_wp == _lp) and _wn or nil
        if _wp == _lp and _eq[_wn] then
            local _dk = self:ToEnum("Data")
            if vmRef[_dk] then
                if _eq[_wn].Skin then
                    vmRef[_dk][self:ToEnum("Skin")] = _eq[_wn].Skin
                    vmRef[_dk][self:ToEnum("Name")] = _eq[_wn].Skin.Name
                end
                if _eq[_wn].Charm then vmRef[_dk][self:ToEnum("Charm")] = _eq[_wn].Charm end
                if _eq[_wn].Wrap  then vmRef[_dk][self:ToEnum("Wrap")]  = _eq[_wn].Wrap  end
            elseif vmRef.Data then
                if _eq[_wn].Skin  then vmRef.Data.Skin  = _eq[_wn].Skin; vmRef.Data.Name = _eq[_wn].Skin.Name end
                if _eq[_wn].Charm then vmRef.Data.Charm = _eq[_wn].Charm end
                if _eq[_wn].Wrap  then vmRef.Data.Wrap  = _eq[_wn].Wrap  end
            end
        end
        local _r = _origCVM(self, vmRef)
        _buildingWep = nil
        return _r
    end
end

local _vmMod = _lp.PlayerScripts.Modules.ClientReplicatedClasses.ClientFighter.ClientItem:FindFirstChild("ClientViewModel")
if _vmMod then
    local _CVM = require(_vmMod)
    local _origNew = _CVM.new
    _CVM.new = function(repData, cliItm)
        local _wp  = cliItm.ClientFighter and cliItm.ClientFighter.Player
        local _wn  = _buildingWep or cliItm.Name
        if _wp == _lp and _eq[_wn] then
            local _RC  = require(_rs.Modules.ReplicatedClass)
            local _dk  = _RC:ToEnum("Data")
            repData[_dk] = repData[_dk] or {}
            local _cos = _eq[_wn]
            if _cos.Skin  then repData[_dk][_RC:ToEnum("Skin")]  = _cos.Skin  end
            if _cos.Charm then repData[_dk][_RC:ToEnum("Charm")] = _cos.Charm end
            if _cos.Wrap  then repData[_dk][_RC:ToEnum("Wrap")]  = _cos.Wrap  end
        end
        return _origNew(repData, cliItm)
    end
end

return "Unlock All active"
