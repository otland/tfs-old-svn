function onSay(cid, words, param, channel)
	local effect = CONST_ME_MAGIC_RED
	local ret = doSummonMonster(cid, param)
	if(ret ~= RETURNVALUE_NOERROR) then
		effect = CONST_ME_POFF
		doPlayerSendDefaultCancel(cid, ret)
	end

	doSendMagicEffect(getCreaturePosition(cid), effect)
	return TRUE
end
