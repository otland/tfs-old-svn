function onSay(cid, words, param)
	if param ~= nil then
		local seperator = string.find(param, ", ")
		if seperator ~= nil then
			local text = string.sub(param, 1, seperator - 1)
			local color = tonumber(string.sub(param, seperator + 2, string.len(param)))
			if(color > 0 and color < 255) then
				doSendAnimatedText(getCreaturePosition(cid), text, color)
			else
				doPlayerSendCancel(cid, "Typed color has to be between 0 and 255")
			end
		else
			doPlayerSendCancel(cid, "You need to type color.")
		end
	else
		doPlayerSendCancel(cid, "You need to type text.")
	end
	return FALSE
end
