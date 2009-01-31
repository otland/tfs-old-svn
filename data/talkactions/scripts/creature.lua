function onSay(cid, words, param)
	local func = doCreateMonster
	if(words:sub(2, 2) == "n") then
		func = doCreateNpc
	end

	local position = getCreaturePosition(cid)
	local effect = CONST_ME_MAGIC_RED
	if(func(param, position) == LUA_ERROR) then
		effect = CONST_ME_POFF
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHROOM)
	end

	doSendMagicEffect(position, effect)
	return TRUE
end
