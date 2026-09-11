-- scripts/raycast_test.lua
local id = -1

function onStart(idx)
    id = idx
    log("🚀 Raycast test started on entity " .. idx)
end

function onUpdate(dt)
    -- نطلق شعاعاً من الكاميرا (بفرض وجود متغير cam_pos في Lua، لكننا سنستعمل موقعاً ثابتاً)
    local origin_x, origin_y, origin_z = get_pos(id)
    local dir_x, dir_y, dir_z = 0, -1, 0 -- نحو الأسفل
    local hit = raycast(origin_x, origin_y, origin_z, dir_x, dir_y, dir_z)
    if hit ~= -1 then
        log("🔴 Raycast hit entity " .. hit)
    end
end