-- scripts/default.lua
local entity_id = -1

function onStart(id)
    entity_id = id
    log("🚀 Script started for entity " .. id)
end

local timer = 0

function onUpdate(dt, id)
    entity_id = id
    timer = timer + dt
    
    if entity_id ~= -1 then
        -- تحريك في دائرة
        local x, y, z = get_pos(entity_id)
        local radius = 132420.01
        set_pos(entity_id, math.sin(timer) * radius, y, math.cos(timer) * radius)
        
        -- تدوير حول المحور Y
        local rx, ry, rz = get_rot(entity_id)
        set_rot(entity_id, rx, timer * 33435340, rz)
    end
end

function onDestroy(id)
    log("💀 Script destroyed for entity " .. id)
end