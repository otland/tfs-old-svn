local colors = {
	["advance"] = ESSAGE_EVENT_ADVANCE,
	["event"] = MESSAGE_EVENT_DEFAULT,
	["orange"] = MESSAGE_STATUS_CONSOLE_ORANGE,
	["info"] = MESSAGE_INFO_DESCR,
	["small"] = MESSAGE_STATUS_SMALL,
	["blue"] = MESSAGE_STATUS_CONSOLE_BLUE,
	["red"] = MESSAGE_STATUS_CONSOLE_RED,
	["warning"] = MESSAGE_STATUS_WARNING,
	["status"] = MESSAGE_STATUS_DEFAULT
}

function onSay(cid, words, param)
	if param ~= nil then
		local seperator = string.find(param, ", ", (string.find(string.lower(param), "[advance,event,orange,info,small,blue,red,warning,status]") + 1))
		if seperator ~= nil then
			doBroadcastMessage(string.sub(param, (seperator + 2), string.len(param)), colors[string.sub(param, 1, seperator - 1)])
		else
			doBroadcastMessage(param)
		end
	end
end