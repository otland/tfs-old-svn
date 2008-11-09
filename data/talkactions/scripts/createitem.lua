function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return FALSE
	end

	local t = string.explode(param, ",")
	local id = tonumber(t[1])
	if(not id) then
		id = getItemIdByName(t[1])
	end

	local amount = 100
	if(t[2]) then
		amount = t[2]
	end

	local ret = doPlayerAddItem(cid, id, amount, 1)
	if(not ret or ret == LUA_ERROR) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Couldn't add item: " .. t[1])
		return FALSE
	end

	doSendMagicEffect(getCreaturePosition(cid), CONST_ME_MAGIC_RED)
	return TRUE
end
