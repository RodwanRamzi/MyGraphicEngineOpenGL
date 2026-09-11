-- scripts/player.lua
local player_id = -1
local speed = 0.2

function onStart(id)
    player_id = id
    log("🟢 Player spawned with ID: " .. id)
    -- تأكد من أن الكاميرا تتبع هذا اللاعب
    camera_set_target(player_id)
end

function onUpdate(dt)
    if player_id == -1 then return end

    -- الحصول على زاوية انعطاف الكاميرا (yaw)
    local yaw = get_camera_yaw()
    local yawRad = math.rad(yaw)

    -- حساب اتجاهات الحركة نسبة للكاميرا (أفقية فقط)
    local forwardX = -math.sin(yawRad)
    local forwardZ = -math.cos(yawRad)
    local rightX = math.cos(yawRad)
    local rightZ = -math.sin(yawRad)

    -- قراءة المدخلات
    local moveX = 0
    local moveZ = 0
    if is_key_pressed("W") then
        moveX = moveX + forwardX * speed * dt
        moveZ = moveZ + forwardZ * speed * dt
    end
    if is_key_pressed("S") then
        moveX = moveX - forwardX * speed * dt
        moveZ = moveZ - forwardZ * speed * dt
    end
    if is_key_pressed("A") then
        moveX = moveX - rightX * speed * dt
        moveZ = moveZ - rightZ * speed * dt
    end
    if is_key_pressed("D") then
        moveX = moveX + rightX * speed * dt
        moveZ = moveZ + rightZ * speed * dt
    end

    -- تطبيق الحركة
    if moveX ~= 0 or moveZ ~= 0 then
        local x, y, z = get_pos(player_id)
        set_pos(player_id, x + moveX, y, z + moveZ)
    end

    -- تدوير اللاعب لمواجهة اتجاه الكاميرا
    -- set_rot(player_id, 0, yaw, 0)
end