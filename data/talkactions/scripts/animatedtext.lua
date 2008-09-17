function onSay(cid, words, param)
	if param ~= nil then
		local seperator = string.find(param, ", ")
		if seperator ~= nil then
			local text = string.sub(param, 1, seperator -1 )
			local color = string.sub(value, seperator + 2, string.len(value))

			doSendAnimatedText(getCreaturePosition(cid), text, color)
		else
			doPlayerSendCancel(cid, "You need to type an effect.")
		end
	else
		doPlayerSendCancel(cid, "You need to type text.")
	end
	return FALSE
end
