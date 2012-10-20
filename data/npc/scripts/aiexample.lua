_G['NpcSystem'] = nil
_G['NpcHandler'] = nil
_G['KeywordHandler'] = nil
_G['KeywordNode'] = nil
_G['Queue'] = nil
_G['Modules'] = nil
_G['StdModule'] = nil
_G['FocusModule'] = nil
_G['KeywordModule'] = nil
_G['TravelModule'] = nil
_G['OutfitModule'] = nil
_G['ShopModule'] = nil

local Npc = AI.SocialNpc:New()
 
Npc.VisionRadius = 5
Npc.HearRadius = 3
Npc.LookAtFocus = false
 
-- All of these properties have their own default values, just messing around
Npc.GreetKeywords = {"hi", "sup"}
Npc.FarewellKeywords = {"bb", "cYa"}
Npc.MaxIdleTime = 10000
Npc.ResponseDelay = 1000
Npc.TalkCaseSensitive = true
Npc.PlayerNameToken = 'DUDE'
 
 
Npc.GreetReply = "Hello |DUDE|, I'm a test npc"
Npc.FarewellReply = "Bye |DUDE|"
 
Npc.Conversation = {
	{["ass%"]= function(talk, match)
		talk:Say("You called me an " .. match)
		talk:Say("You must die")
 
		local m = talk:Listen()
		talk:End("And now you called me a " .. m .. "! Bye |DUDE|")
	end},
 
	{["job|occupation"] = "My job is to be a !failure"},
	{["mission|quest"] = "I have no missions for you son"},
	{["heal"] = {
		{[function(talk) 
			return getCreatureHealth(talk.player) < 200 
		end] = function(talk)
			doCreatureAddHealth(talk.player, 200)
			return "You have been healed!"
		end},
		{[true] = "Sorry ur not wounded enought!"}
	}}
}
 
Npc:Register()