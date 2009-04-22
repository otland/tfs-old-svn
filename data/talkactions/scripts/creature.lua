function onSay(cid, words, param, channel)
	local func = doCreateMonster
	if(words:sub(2, 2) == "n") then
		func = doCreateNpc
	end

	local position = getCreaturePosition(cid)
	local effect = CONST_ME_MAGIC_RED
	local ret = func(param, position, FALSE)
	if(tonumber(ret) == nil) then
		effect = CONST_ME_POFF
		doPlayerSendDefaultCancel(cid, (ret == LUA_ERROR and RETURNVALUE_NOTPOSSIBLE or RETURNVALUE_NOTENOUGHROOM))
	end

	doSendMagicEffect(position, effect)
	return TRUE
end
