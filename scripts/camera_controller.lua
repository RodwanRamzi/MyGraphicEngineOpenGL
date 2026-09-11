-- scripts/camera_controller.lua
local camera_id = -1
local target_id = -1

function onStart(id)
    camera_id = id
    log("📷 Camera controller started for ID: " .. id)
    
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
    
    -- ضبط موقع الكاميرا (مثال: شخص ثالث خلف اللاعب)
    -- سنستخدم زاوية yaw من الكاميرا (أو من اللاعب)
    local yaw = get_camera_yaw()  -- إذا كانت متوفرة
    -- أو نستخدم دوران اللاعب
    -- local rx, ry, rz = get_rot(target_id)
    -- local yaw = ry
    
    local distance = 4.0
    local height = 2.0
    local yawRad = math.rad(yaw)
    
    local cx = px - math.sin(yawRad) * distance
    local cy = py + height
    local cz = pz - math.cos(yawRad) * distance
    
    set_pos(camera_id, cx, cy, cz)
    
    -- تدوير الكاميرا لتنظر إلى اللاعب
    local dx = px - cx
    local dy = py - cy
    local dz = pz - cz
    local angleY = math.deg(math.atan2(dx, dz))
    local angleX = math.deg(math.atan2(dy, math.sqrt(dx*dx + dz*dz)))
    set_rot(camera_id, angleX, angleY, 0)
end