-- scripts/camera_zone.lua
-- هذا السكريبت يُرفق بكيان المنطقة (Zone) لتبديل الكاميرا عند الدخول

local zone_id = -1
local camera_id = -1      -- الكاميرا التي سيتم التبديل إليها
local player_id = -1
local radius = 3.0        -- نصف قطر المنطقة (مربع أو كرة)
local is_player_inside = false
local previous_camera = -1

function onStart(id)
    zone_id = id
    log("🔵 Camera Zone started at ID: " .. id)
    
    -- البحث عن اللاعب
    for i = 0, 50 do
        if get_entity_type(i) == "Player" then
            player_id = i
            log("🎯 Zone found player at ID: " .. i)
            break
        end
    end
    
    -- البحث عن الكاميرا المرتبطة (نفترض أنها الكيان التالي أو نمررها كمعامل)
    -- هنا نفترض أن الكاميرا هي الكيان التالي في القائمة (أو يمكن تمرير ID)
    -- لكن سنستخدم دالة خاصة لتحديدها
end

-- دالة لربط الكاميرا بهذه المنطقة
function set_target_camera(cam_id)
    camera_id = cam_id
    log("📷 Zone target camera set to ID: " .. cam_id)
end

function onUpdate(dt)
    if player_id == -1 then return end
    if camera_id == -1 then 
        log("⚠️ Zone has no target camera set!")
        return 
    end
    
    -- الحصول على موقع اللاعب والمنطقة
    local px, py, pz = get_pos(player_id)
    local zx, zy, zz = get_pos(zone_id)
    
    -- حساب المسافة (في المستوى XY فقط للتبسيط، أو ثلاثية الأبعاد)
    local dx = px - zx
    local dz = pz - zz
    local dist = math.sqrt(dx*dx + dz*dz)
    
    -- التحقق من الدخول إلى المنطقة
    if dist < radius then
        if not is_player_inside then
            -- اللاعب دخل إلى المنطقة
            is_player_inside = true
            log("🔵 Player entered camera zone!")
            
            -- حفظ الكاميرا الحالية (للعودة لاحقاً)
            previous_camera = get_active_camera()
            
            -- التبديل إلى الكاميرا المستهدفة
            set_active_camera(camera_id)
            log("📷 Switched to camera ID: " .. camera_id)
        end
    else
        if is_player_inside then
            -- اللاعب خرج من المنطقة
            is_player_inside = false
            log("🔴 Player left camera zone!")
            
            -- العودة إلى الكاميرا السابقة (إذا كانت موجودة)
            if previous_camera ~= -1 then
                set_active_camera(previous_camera)
                log("📷 Returned to camera ID: " .. previous_camera)
            end
        end
    end
end