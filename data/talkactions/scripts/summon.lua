function onSay(cid, words, param)
	local effect = CONST_ME_MAGIC_RED
	if(doSummonMonster(cid, param) ~= RETURNVALUE_NOERROR) then
		effect = CONST_ME_POFF
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHROOM)
	end

	doSendMagicEffect(getCreaturePosition(cid), effect)
	return TRUE
end
