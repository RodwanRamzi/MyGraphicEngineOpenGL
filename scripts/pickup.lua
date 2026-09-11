-- scripts/pickup.lua
local pickup_id = -1
local player_id = -1
local collected = false
local timer = 0
local respawn_time = 3.0

function onStart(id)
    pickup_id = id
    log("🟡 Pickup spawned with ID: " .. id)
end

function onUpdate(dt)
    if is_game_over() then return end
    
    -- البحث عن اللاعب
    if player_id == -1 then
        for i = 0, 50 do
            if get_entity_type(i) == "Player" then
                player_id = i
                log("🟡 Pickup found player at ID: " .. i)
                break
            end
        end
    end
    
    if player_id == -1 then return end
    
    if collected then
        timer = timer + dt
        if timer >= respawn_time then
            collected = false
            timer = 0
            -- إعادة الظهور في مكان عشوائي
            local rand_x = math.random(-10, 10)
            local rand_z = math.random(-10, 10)
            set_pos(pickup_id, rand_x, 0.5, rand_z)
            log("🟡 Pickup respawned at (" .. rand_x .. ", 0.5, " .. rand_z .. ")")
        end
        return
    end
    
    -- تدوير الهدف
    local rx, ry, rz = get_rot(pickup_id)
    set_rot(pickup_id, rx, ry + dt * 60, rz)
    
    -- التحقق من التقاط الهدف
    local px, py, pz = get_pos(player_id)
    local ex, ey, ez = get_pos(pickup_id)
    local dist = math.sqrt((px-ex)^2 + (pz-ez)^2)
    
    if dist < 1.5 then
        collected = true
        timer = 0
        local score = get_score() + 10
        set_entity_type(pickup_id, "Static") -- إعادة تعيين النوع (اختياري)
        add_score(10)
        log("🪙 +10 points! Total: " .. score)
        -- إخفاء الهدف
        set_pos(pickup_id, 0, -100, 0)
    end
end