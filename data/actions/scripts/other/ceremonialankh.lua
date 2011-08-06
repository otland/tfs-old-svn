local blessings = {"\nWisdom of Solitude", "\nSpark of the Phoenix", "\nFire of the Suns", "\nSpiritual Shielding", "\nEmbrace of Tibia", "\nTwist of Fate"}
function onUse(cid, item, fromPosition, itemEx, toPosition)
	local result = "Received blessings:"
	for i = 1, 5 do
		result = getPlayerBlessing(cid, i) and result .. blessings[i] or result
	end
	if getPlayerPVPBlessing(cid) then
		result = result..blessings[6]
	end
	return doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, 20 > result:len() and "No blessings received." or result)
end
