function onUse(cid, item, fromPosition, itemEx, toPosition)
	if(itemEx.itemid == FALSE) then
		return FALSE
	end

	if(getPlayerLevel(cid) >= 2) then
		local found = FALSE
		local skin = 0

		if(isInArray({2830, 2871, 2866, 2876, 3090}, itemEx.itemid) == TRUE) then
			found = TRUE
			if(math.random(1,5) == 1) then
				skin = 5878
			end
		elseif(isInArray({4259, 4262, 4256}, itemEx.itemid) == TRUE) then
			found = TRUE
			if(math.random(1,5) == 1) then
				skin = 5876
			end
		elseif(isInArray({3104, 2844}, itemEx.itemid) == TRUE) then
			found = TRUE
			if(math.random(1,5) == 1) then
				skin = 5877
			end
		elseif(isInArray({2881}, itemEx.itemid) == TRUE) then
			found = TRUE
			if(math.random(1,5) == 1) then
				skin = 5948
			end
		elseif(isInArray({2931}, itemEx.itemid) == TRUE) then
			found = TRUE
			local rand = math.random(1,10)
			if(rand > 8) then
				skin = 5893
			elseif(rand == 4) then
				skin = 5930
			end
		elseif(isInArray({3031}, itemEx.itemid) == TRUE) then
			found = TRUE
			if(math.random(1,5) == 1) then
				skin = 5925
			end
		end

		if(found == TRUE) then
			if(skin == 0) then
				doSendMagicEffect(toPosition, CONST_ME_POFF)
			else
				doSendMagicEffect(toPosition, CONST_ME_GROUNDSHAKER)
				doPlayerAddItem(cid, skin, 1)
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