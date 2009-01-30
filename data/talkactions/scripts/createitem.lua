function onSay(cid, words, param)
	if(param == "") then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return TRUE
	end

	local t = string.explode(param, ",")
	local ret = RETURNVALUE_NOERROR
	local tmp = getCreaturePosition(cid)

	local id = tonumber(t[1])
	if(not id) then
		id = getItemIdByName(t[1], FALSE)
		if(id == LUA_ERROR) then
			doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Item wich such name does not exists.")
			return TRUE
		end
	end

	local amount = 100
	if(t[2]) then
		amount = t[2]
	end

	local item = doCreateItemEx(id, amount)
	if(t[3] and getBooleanFromString(t[3]) == TRUE) then
		if(t[4] and getBooleanFromString(t[4]) == TRUE) then
			tmp = getPlayerLookPos(cid)
		end

		ret = doTileAddItemEx(tmp, item)	
	else
		ret = doPlayerAddItemEx(cid, item, TRUE)
	end

	if(ret ~= RETURNVALUE_NOERROR) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Couldn't add item: " .. t[1])
		return TRUE
	end

	doSendMagicEffect(tmp, CONST_ME_MAGIC_RED)
	return TRUE
end
