-- scripts/setup_camera.lua
log("📷 Setting up camera...")

-- 1. البحث عن اللاعب (أو تمرير ID مباشرة)
local player_id = -1
for i = 0, 50 do
    if get_entity_type(i) == "Player" then
        player_id = i
        break
    end
end

if player_id == -1 then
    log("❌ Player not found! Cannot setup camera.")
    return
end

-- 2. تعيين الهدف (اللاعب)
camera_set_target(player_id)

-- 3. اختيار الوضع (شخص أول أو ثالث)
-- ----- وضع الشخص الثالث (Third Person) -----
camera_set_distance(4.0)      -- المسافة خلف اللاعب
camera_set_height(2.0)        -- الارتفاع عن الأرض
camera_set_pitch(-20)         -- زاوية الميل (نظرة منحدرة)
camera_set_yaw(0)             -- زاوية الانعطاف (يسار/يمين)
camera_set_smooth_speed(5.0)  -- سرعة الانتقال السلس
camera_set_mode(true)         -- lookAtTarget = true (تنظر إلى اللاعب)

-- ----- وضع الشخص الأول (FPS) - قم بتعليق الأسطر أعلاه واستخدام هذه -----
-- camera_set_distance(0.0)
-- camera_set_height(1.5)
-- camera_set_pitch(0)
-- camera_set_yaw(0)
-- camera_set_smooth_speed(10.0)
-- camera_set_mode(true)

-- عرض معلومات التهيئة
log("✅ Camera configured!")
log("📷 Target: Player ID " .. player_id)
log("📏 Distance: " .. camera_get_distance())
log("📐 Height: " .. camera_get_height())
log("🎯 Pitch: " .. camera_get_pitch() .. "°")
log("🔄 Yaw: " .. camera_get_yaw() .. "°")
log("⚡ Smooth Speed: " .. camera_get_smooth_speed())