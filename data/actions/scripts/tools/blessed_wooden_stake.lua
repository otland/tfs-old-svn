function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(itemEx.itemid == FALSE) then
		return FALSE
	end

	if(getPlayerLevel(cid) >= 2) then
		local found = FALSE
		local dust = 0

		if(isInArray({2956}, itemEx.itemid) == TRUE) then
			found = TRUE
			if(math.random(1,5) == 1) then
				dust = 5905
			end
		elseif(isInArray({2916}, itemEx.itemid) == TRUE) then
			found = TRUE
			if(math.random(1,5) == 1) then
				dust = 5906
			end
		end

		if(found == TRUE) then
			if(dust == 0) then
				doSendMagicEffect(toPosition, CONST_ME_POFF)
			else
				doSendMagicEffect(toPosition, CONST_ME_GROUNDSHAKER)
				doPlayerAddItem(cid, dust, 1)
				doSendAnimatedText(fromPosition, 'Success!', TEXTCOLOR_WHITE_EXP);
			end
			doTransformItem(itemEx.uid, itemEx.itemid + 1)
		else
			doPlayerSendCancel(cid, "Sorry, not possible.")
		end
	else
		doPlayerSendCancel(cid, "You have to be at least Level 2 to use this tool.")
	end

	return TRUE
end