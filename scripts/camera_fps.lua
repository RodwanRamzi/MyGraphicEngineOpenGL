-- scripts/camera_fps.lua
local camera_id = -1
local target_id = -1

function onStart(id)
    camera_id = id
    log("📷 FPS Camera started at ID: " .. id)
    
    -- البحث عن اللاعب
    for i = 0, 50 do
        if get_entity_type(i) == "Player" then
            target_id = i
            log("🎯 Camera targeting player at ID: " .. i)
            break
        end
    end
end

function onUpdate(dt)
    if target_id == -1 then return end
    
    -- الحصول على موقع اللاعب
    local px, py, pz = get_pos(target_id)
    
    -- وضع الكاميرا في عين اللاعب (ارتفاع 1.5 متر)
    local eye_height = 1.5
    set_pos(camera_id, px, py + eye_height, pz)
    
    -- تدوير الكاميرا بنفس زوايا اللاعب
    local rx, ry, rz = get_rot(target_id)
    set_rot(camera_id, rx, ry, rz)
end