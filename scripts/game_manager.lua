-- scripts/game_manager.lua
local start_time = get_time()
local game_duration = 60 -- ثانية
local win_score = 50

function onStart()
    log("Game Manager started!")
    log("You have " .. game_duration .. " seconds to collect " .. win_score .. " points.")
    log("Use WASD to move and avoid enemies!")
end

function onUpdate(dt)
    if is_game_over() then return end
    if is_game_won() then return end
    
    -- التحقق من الوقت
    local elapsed = get_time() - start_time
    local remaining = game_duration - elapsed
    
    if remaining <= 0 then
        -- انتهى الوقت
        if get_score() >= win_score then
            log("You won! Final score: " .. get_score())
            -- يمكن إظهار شاشة فوز
        else
            log("Time's up! Score: " .. get_score() .. " (Need " .. win_score .. ")")
            -- يمكن إظهار شاشة خسارة
        end
    end
    
    -- التحقق من الفوز
    if get_score() >= win_score and not is_game_won() then
        log("Congratulations! You won with " .. get_score() .. " points!")
        -- يمكن إظهار شاشة فوز
    end
    
    -- التحقق من الخسارة
    if get_health() <= 0 and not is_game_over() then
        log("Game Over! You died.")
        -- يمكن إظهار شاشة خسارة
    end
end