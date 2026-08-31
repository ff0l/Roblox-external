-- FF0L bridge. Put this in your executor autoexec once.
-- FF0L writes scripts to %AppData%\ff0l and signals runs through bridge\cmd.

local HttpService = game:GetService("HttpService")

local function appRoot()
    local app = os.getenv("APPDATA")
    if not app or app == "" then
        return nil
    end
    return app .. "\\ff0l"
end

local function readText(path)
    if not isfile or not readfile then
        return nil
    end
    local ok, exists = pcall(isfile, path)
    if not ok or not exists then
        return nil
    end
    local okRead, body = pcall(readfile, path)
    if not okRead then
        return nil
    end
    return body
end

local function writeText(path, body)
    if not writefile then
        return false
    end
    local ok = pcall(writefile, path, body or "")
    return ok
end

local function runUnlock(root)
    local scriptPath = root .. "\\scripts\\rivals_unlock.lua"
    local source = readText(scriptPath)
    if not source or source == "" then
        writeText(root .. "\\bridge\\status", "missing rivals_unlock.lua")
        return
    end
    local ok, result = pcall(function()
        local chunk = loadstring(source, "rivals_unlock")
        if not chunk then
            error("compile failed")
        end
        return chunk()
    end)
    if ok then
        local message = typeof(result) == "string" and result or "Unlock All active"
        writeText(root .. "\\bridge\\status", message)
    else
        writeText(root .. "\\bridge\\status", tostring(result))
    end
end

local root = appRoot()
if not root then
    return
end

if makefolder then
    pcall(makefolder, root .. "\\bridge")
end

writeText(root .. "\\bridge\\heartbeat", HttpService:GenerateGUID(false))

while true do
    local cmd = readText(root .. "\\bridge\\cmd")
    if cmd and cmd ~= "" then
        cmd = string.gsub(cmd, "%s+", "")
        if cmd == "unlock" then
            runUnlock(root)
        end
        writeText(root .. "\\bridge\\cmd", "")
    end
    task.wait(0.25)
end
