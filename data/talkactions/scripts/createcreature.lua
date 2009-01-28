function onSay(cid, words, param)
	local func = doCreateMonster
	if(words:sub(2, 2) == "n") then
		func = doCreateNpc
	end

	local position = getCreaturePosition(cid)
	local effect = CONST_ME_POFF
	if(func(param, position) ~= LUA_ERROR)
		effect = CONST_ME_BLOOD
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHROOM)
	end

	doSendMagicEffect(position, effect)
	return TRUE
end
