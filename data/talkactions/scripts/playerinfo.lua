function onSay(cid, words, param, channel)
	if(param == '') then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Command param required.")
		return true
	end

	local pid = getPlayerByNameWildcard(param)
	if(not pid or (isPlayerGhost(pid) and getPlayerGhostAccess(pid) > getPlayerGhostAccess(cid))) then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Player " .. param .. " not found.")
		return true
	end

	local tmp = {accountId = getPlayerAccountId(pid), ip = getPlayerIp(pid)}
	local pos = getCreaturePosition(pid)
	doPlayerPopupFYI(cid, "Information about player" ..
		"\nName: " .. getCreatureName(pid) ..
		"\nGUID: " .. getPlayerGUID(pid) ..
		"\nGroup: " .. getPlayerGroupName(pid) ..
		"\nAccess: " .. getPlayerAccess(pid) ..
		"\nVocation: " .. getVocationInfo(getPlayerVocation(pid)).name ..
		"\nStatus:" ..
			"\nLevel - " .. getPlayerLevel(pid) .. ", Magic Level - " .. getPlayerMagLevel(pid) .. ", Speed - " .. getCreatureSpeed(pid) ..
			"\nHealth - " .. getCreatureHealth(pid) .. " / " .. getCreatureMaxHealth(pid) .. ", Mana - " .. getCreatureMana(pid) .. " / " .. getCreatureMaxMana(pid) ..
			"\nSkills:" ..
			"\nFist - " .. getPlayerSkillLevel(pid, SKILL_FIST) .. ", Club - " .. getPlayerSkillLevel(pid, SKILL_CLUB) .. ", Sword - " .. getPlayerSkillLevel(pid, SKILL_SWORD) .. ", Axe - " .. getPlayerSkillLevel(pid, SKILL_AXE) ..
			"\nDistance - " .. getPlayerSkillLevel(pid, SKILL_DISTANCE) .. ", Shielding - " .. getPlayerSkillLevel(pid, SKILL_SHIELD) .. ", Fishing - " .. getPlayerSkillLevel(pid, SKILL_FISHING) ..
		"\nCash:" ..
			"\nCrystal - " .. getPlayerItemCount(pid, 2160) .. ", Platinum - " .. getPlayerItemCount(pid, 2152) .. ", Gold - " .. getPlayerItemCount(pid, 2148) ..
			"\nBalance: " .. getPlayerBalance(pid) ..
			"\nPosition: [X - " .. pos.x .. " | Y - " .. pos.y .. " | Z - " .. pos.z .. "]" ..
		"\n\nInformation about account" ..
		"\nName: " .. getPlayerAccount(pid) ..
		"\nID: " .. tmp.accountId ..
		"\nNotations: " .. getNotationsCount(tmp.accountId) ..
		"\nIP: " .. doConvertIntegerToIp(tmp.ip) .. " (" .. tmp.ip .. ")")
	return true
end
