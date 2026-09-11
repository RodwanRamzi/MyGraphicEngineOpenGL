-- scripts/enemy.lua
local enemy_id = -1
local player_id = -1
local speed = 2.0
local damage = 10
local attack_cooldown = 0
local search_radius = 20.0

function onStart(id)
    enemy_id = id
    log("🔴 Enemy spawned with ID: " .. id)
end

function onUpdate(dt)
    if is_game_over() then return end
    
    -- البحث عن اللاعب في كل إطار (أكثر موثوقية)
    if player_id == -1 then
        for i = 0, 50 do
            local typ = get_entity_type(i)
            if typ == "Player" then
                player_id = i
                log("🎯 Enemy found player at ID: " .. i)
                break
            end
        end
    end
    
    if player_id == -1 then
        -- لا يوجد لاعب، ننتظر
        return
    end
    
    -- الحصول على مواقع اللاعب والعدو
    local ex, ey, ez = get_pos(enemy_id)
    local px, py, pz = get_pos(player_id)
    
    -- حساب المسافة
    local dx = px - ex
    local dz = pz - ez
    local dist = math.sqrt(dx*dx + dz*dz)
    
    -- إذا كان اللاعب ضمن نطاق البحث
    if dist < search_radius and dist > 0.5 then
        -- التحرك نحو اللاعب
        local move_speed = speed * dt
        local new_x = ex + (dx / dist) * move_speed
        local new_z = ez + (dz / dist) * move_speed
        set_pos(enemy_id, new_x, ey, new_z)
        
        -- تدوير العدو ليواجه اللاعب
        local angle = math.atan2(dx, dz) * (180 / math.pi)
        set_rot(enemy_id, 0, angle, 0)
    end
    
    -- إذا لامس العدو اللاعب
    if dist < 1.2 then
        attack_cooldown = attack_cooldown - dt
        if attack_cooldown <= 0 then
            -- إصابة اللاعب
            local health = get_health() - damage
            set_health(health)
            log("💢 Enemy attacked! Health: " .. health)
            
            -- دفع العدو للخلف قليلاً
            local push_back = 2.0
            local new_x = ex - (dx / dist) * push_back
            local new_z = ez - (dz / dist) * push_back
            set_pos(enemy_id, new_x, ey, new_z)
            
            attack_cooldown = 1.5
        end
    end
end