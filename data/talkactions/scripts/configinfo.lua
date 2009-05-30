local HIDDEN = {"passwordType", "sql"}

local function cancelInfo(cid, value)
	doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Config value " .. value .. " doesn't exists.")
end

function onSay(cid, words, param, channel)
	for _, str in ipairs(HIDDEN) do
		if(param:find(str) ~= nil) then
			cancelInfo(cid, param)
			return true
		end
	end

	local value = getConfigValue(param)
	if(value) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, param .. " = " .. value)
	else
		cancelInfo(cid, param)
	end

	return true
end
