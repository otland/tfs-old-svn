CREATE TABLE "server_config"
(
	"config" VARCHAR(35) NOT NULL DEFAULT '',
	"value" INT NOT NULL,
	UNIQUE ("config")
);

INSERT INTO "server_config" VALUES ('db_version', 7);

CREATE TABLE "server_motd"
(
	"id" SERIAL,
	"text" TEXT NOT NULL,
	PRIMARY KEY ("id")
);

INSERT INTO "server_motd" VALUES (1, 'Welcome to The Forgotten Server!');

CREATE TABLE "server_record"
(
	"record" INT NOT NULL,
	"timestamp" BIGINT NOT NULL,
	UNIQUE ("timestamp", "record")
);

INSERT INTO "server_record" VALUES (0, 0);

CREATE TABLE "server_reports"
(
	"id" SERIAL,
	"player_id" INT NOT NULL DEFAULT 0,
	"posx" INT NOT NULL DEFAULT 0,
	"posy" INT NOT NULL DEFAULT 0,
	"posz" INT NOT NULL DEFAULT 0,
	"timestamp" BIGINT NOT NULL DEFAULT 0,
	"report" TEXT NOT NULL,
	"reads" INT NOT NULL DEFAULT 0,
	PRIMARY KEY ("id")
);

CREATE TABLE "groups"
(
	"id" SERIAL,
	"name" VARCHAR(255) NOT NULL,
	"flags" BIGINT NOT NULL DEFAULT 0,
	"customflags" BIGINT NOT NULL DEFAULT 0,
	"access" INT NOT NULL,
	"violationaccess" INT NOT NULL,
	"maxdepotitems" INT NOT NULL,
	"maxviplist" INT NOT NULL,
	"outfit" INT NOT NULL DEFAULT 0,
	PRIMARY KEY ("id")
);

INSERT INTO "groups" VALUES (nextval('groups_id_seq'::regclass), 'Player', 0, 0, 0, 0, 0, 0, 0);
INSERT INTO "groups" VALUES (nextval('groups_id_seq'::regclass), 'Tutor', 16809984, 524291, 1, 0, 0, 0, 0);
INSERT INTO "groups" VALUES (nextval('groups_id_seq'::regclass), 'Senior Tutor', 68736352256, 524303, 2, 1, 0, 0, 0);
INSERT INTO "groups" VALUES (nextval('groups_id_seq'::regclass), 'Gamemaster', 510024081247, 4189375, 3, 2, 4000, 200, 75);
INSERT INTO "groups" VALUES (nextval('groups_id_seq'::regclass), 'Community Manager', 542239465466, 4189695, 4, 3, 6000, 300, 266);
INSERT INTO "groups" VALUES (nextval('groups_id_seq'::regclass), 'God', 546534563834, 4194303, 5, 3, 8000, 400, 302);

CREATE TABLE "accounts"
(
	"id" SERIAL,
	"name" VARCHAR(32) NOT NULL DEFAULT '',
	"password" VARCHAR(255) NOT NULL,
	"premdays" INT NOT NULL DEFAULT 0,
	"lastday" INT NOT NULL DEFAULT 0,
	"email" VARCHAR(255) NOT NULL DEFAULT '',
	"key" VARCHAR(20) NOT NULL DEFAULT '0',
	"blocked" SMALLINT NOT NULL DEFAULT 0,
	"warnings" INT NOT NULL DEFAULT 0,
	"group_id" INT NOT NULL DEFAULT 1,
	PRIMARY KEY ("id"), UNIQUE ("name"),
	FOREIGN KEY ("group_id") REFERENCES "groups" ("id")
);

INSERT INTO "accounts" VALUES (nextval('accounts_id_seq'::regclass), '1', '1', 65535, 0, '', '0', 0, 0, 1);

CREATE TABLE "players"
(
	"id" SERIAL,
	"name" VARCHAR(255) NOT NULL,
	"group_id" INT NOT NULL DEFAULT 1,
	"account_id" INT NOT NULL DEFAULT 0,
	"level" INT NOT NULL DEFAULT 1,
	"vocation" INT NOT NULL DEFAULT 0,
	"health" INT NOT NULL DEFAULT 150,
	"healthmax" INT NOT NULL DEFAULT 150,
	"experience" BIGINT NOT NULL DEFAULT 0,
	"lookbody" INT NOT NULL DEFAULT 0,
	"lookfeet" INT NOT NULL DEFAULT 0,
	"lookhead" INT NOT NULL DEFAULT 0,
	"looklegs" INT NOT NULL DEFAULT 0,
	"looktype" INT NOT NULL DEFAULT 136,
	"lookaddons" INT NOT NULL DEFAULT 0,
	"maglevel" INT NOT NULL DEFAULT 0,
	"mana" INT NOT NULL DEFAULT 0,
	"manamax" INT NOT NULL DEFAULT 0,
	"manaspent" INT NOT NULL DEFAULT 0,
	"soul" INT NOT NULL DEFAULT 0,
	"town_id" INT NOT NULL DEFAULT 0,
	"posx" INT NOT NULL DEFAULT 0,
	"posy" INT NOT NULL DEFAULT 0,
	"posz" INT NOT NULL DEFAULT 0,
	"conditions" BYTEA NOT NULL,
	"cap" INT NOT NULL DEFAULT 0,
	"sex" INT NOT NULL DEFAULT 0,
	"lastlogin" BIGINT NOT NULL DEFAULT 0,
	"lastip" INT NOT NULL DEFAULT 0,
	"save" SMALLINT NOT NULL DEFAULT 1,
	"redskull" SMALLINT NOT NULL DEFAULT 0,
	"redskulltime" BIGINT NOT NULL DEFAULT 0,
	"rank_id" INT NOT NULL DEFAULT 0,
	"guildnick" VARCHAR(255) NOT NULL DEFAULT '',
	"lastlogout" BIGINT NOT NULL DEFAULT 0,
	"blessings" SMALLINT NOT NULL DEFAULT 0,
	"balance" BIGINT NOT NULL DEFAULT 0,
	"stamina" BIGINT NOT NULL DEFAULT 201660000,
	"direction" INT NOT NULL DEFAULT 2,
	"loss_experience" INT NOT NULL DEFAULT 10,
	"loss_mana" INT NOT NULL DEFAULT 10,
	"loss_skills" INT NOT NULL DEFAULT 10,
	"loss_items" INT NOT NULL DEFAULT 10,
	"premend" INT NOT NULL DEFAULT 0,
	"online" SMALLINT NOT NULL DEFAULT 0,
	"marriage" INT NOT NULL DEFAULT 0,
	"promotion" INT NOT NULL DEFAULT 0,
	"deleted" SMALLINT NOT NULL DEFAULT 0,
	PRIMARY KEY ("id"), UNIQUE ("name", "deleted"),
	FOREIGN KEY ("account_id") REFERENCES "accounts"("id") ON DELETE CASCADE,
	FOREIGN KEY ("group_id") REFERENCES "groups"("id")
);

INSERT INTO "players" VALUES (nextval('players_id_seq'::regclass), 'Account Manager', 1, 1, 1, 0, 150, 150, 0, 0, 0, 0, 0, 110, 0, 0, 0, 0, 0, 0, 0, 50, 50, 7, '', 400, 0, 0, 0, 0, 0, 0, 0, '', 0, 0, 0, 201660000, 0, 10, 10, 10, 10, 0, 0, 0, 0, 0);

CREATE TABLE "bans"
(
	"id" SERIAL,
	"type" SMALLINT NOT NULL,
	"value" INT NOT NULL,
	"param" INT NOT NULL DEFAULT 4294967295,
	"active" SMALLINT NOT NULL DEFAULT 1,
	"expires" INT NOT NULL,
	"added" INT NOT NULL,
	"admin_id" INT NOT NULL DEFAULT 0,
	"comment" TEXT NOT NULL,
	"reason" INT NOT NULL DEFAULT 0,
	"action" INT NOT NULL DEFAULT 0,
	PRIMARY KEY  ("id")
);

CREATE TABLE "global_storage"
(
	"key" INT NOT NULL,
	"value" VARCHAR(255) NOT NULL DEFAULT '0',
	UNIQUE ("key")
);

CREATE TABLE "guilds"
(
	"id" SERIAL,
	"name" VARCHAR(255) NOT NULL,
	"ownerid" INT NOT NULL,
	"creationdata" INT NOT NULL,
	"motd" VARCHAR(255) NOT NULL,
	PRIMARY KEY ("id"),
	UNIQUE ("name")
);

CREATE TABLE "guild_invites"
(
	"player_id" INT NOT NULL DEFAULT 0,
	"guild_id" INT NOT NULL DEFAULT 0,
	UNIQUE ("player_id", "guild_id")
);

CREATE TABLE "guild_ranks"
(
	"id" SERIAL,
	"guild_id" INT NOT NULL,
	"name" VARCHAR(255) NOT NULL,
	"level" INT NOT NULL,
	PRIMARY KEY ("id"),
	FOREIGN KEY ("guild_id") REFERENCES "guilds"("id") ON DELETE CASCADE
);

CREATE TABLE "house_lists"
(
	"house_id" INT NOT NULL,
	"listid" INT NOT NULL,
	"list" TEXT NOT NULL,
	UNIQUE ("house_id", "listid")
);

CREATE TABLE "houses"
(
	"id" SERIAL,
	"owner" INT NOT NULL,
	"paid" INT NOT NULL DEFAULT 0,
	"warnings" INT NOT NULL DEFAULT 0,
	"lastwarning" INT NOT NULL DEFAULT 0,
	"name" VARCHAR(255) NOT NULL,
	"town" INT NOT NULL DEFAULT 0,
	"size" INT NOT NULL DEFAULT 0,
	"price" INT NOT NULL DEFAULT 0,
	"rent" INT NOT NULL DEFAULT 0,
	PRIMARY KEY ("id")
);

CREATE TABLE "player_deaths"
(
	"player_id" INT NOT NULL,
	"time" BIGINT NOT NULL DEFAULT 0,
	"level" INT NOT NULL DEFAULT 1,
	"killed_by" VARCHAR(255) NOT NULL,
	"altkilled_by" VARCHAR(255) NOT NULL,
	FOREIGN KEY ("player_id") REFERENCES "players"("id") ON DELETE CASCADE
);

CREATE TABLE "player_depotitems"
(
	"player_id" INT NOT NULL,
	"depot_id" INT NOT NULL DEFAULT 0,
	"sid" INT NOT NULL,
	"pid" INT NOT NULL DEFAULT 0,
	"itemtype" INT NOT NULL,
	"count" INT NOT NULL DEFAULT 0,
	"attributes" BYTEA NOT NULL,
	UNIQUE ("player_id", "sid"),
	FOREIGN KEY ("player_id") REFERENCES "players"("id") ON DELETE CASCADE
);

CREATE TABLE "player_items"
(
	"player_id" INT NOT NULL DEFAULT 0,
	"pid" INT NOT NULL DEFAULT 0,
	"sid" INT NOT NULL DEFAULT 0,
	"itemtype" INT NOT NULL DEFAULT 0,
	"count" INT NOT NULL DEFAULT 0,
	"attributes" BYTEA NOT NULL,
	UNIQUE ("player_id", "sid"),
	FOREIGN KEY ("player_id") REFERENCES "players"("id") ON DELETE CASCADE
);

CREATE TABLE "player_namelocks"
(
	"name" VARCHAR(255) NOT NULL,
	"new_name" VARCHAR(255) NOT NULL,
	"date" BIGINT NOT NULL
);

CREATE TABLE "player_skills"
(
	"player_id" INT NOT NULL DEFAULT 0,
	"skillid" SMALLINT NOT NULL DEFAULT 0,
	"value" INT NOT NULL DEFAULT 0,
	"count" INT NOT NULL DEFAULT 0,
	UNIQUE ("player_id", "skillid"),
	FOREIGN KEY ("player_id") REFERENCES "players"("id") ON DELETE CASCADE
);

CREATE TABLE "player_spells"
(
	"player_id" INT NOT NULL,
	"name" VARCHAR(255) NOT NULL,
	UNIQUE ("player_id", "name"),
	FOREIGN KEY ("player_id") REFERENCES "players"("id") ON DELETE CASCADE
);

CREATE TABLE "player_storage"
(
	"player_id" INT NOT NULL DEFAULT 0,
	"key" INT NOT NULL DEFAULT 0,
	"value" VARCHAR(255) NULL DEFAULT '0',
	UNIQUE ("player_id", "key"),
	FOREIGN KEY ("player_id") REFERENCES "players"("id") ON DELETE CASCADE
);

CREATE TABLE "player_viplist"
(
	"player_id" INT NOT NULL,
	"vip_id" INT NOT NULL,
	UNIQUE ("player_id", "vip_id"),
	FOREIGN KEY ("player_id") REFERENCES "players"("id") ON DELETE CASCADE,
	FOREIGN KEY ("vip_id") REFERENCES "players"("id") ON DELETE CASCADE
);

CREATE TABLE "tiles"
(
	"id" INT NOT NULL,
	"x" INT NOT NULL,
	"y" INT NOT NULL,
	"z" SMALLINT NOT NULL,
	UNIQUE ("id")
);

CREATE TABLE "tile_items"
(
	"tile_id" INT NOT NULL,
	"sid" INT NOT NULL,
	"pid" INT NOT NULL DEFAULT 0,
	"itemtype" INT NOT NULL,
	"count" INT NOT NULL DEFAULT 0,
	"attributes" BYTEA NOT NULL,
	UNIQUE ("tile_id", "sid"),
	FOREIGN KEY ("tile_id") REFERENCES "tiles"("id") ON DELETE CASCADE
);

CREATE LANGUAGE "plpgsql";

CREATE FUNCTION "ondelete_accounts"()
RETURNS TRIGGER
AS $$
BEGIN
	DELETE FROM "bans" WHERE "type" NOT IN (1,2) AND "value" = OLD."id";

	RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER "ondelete_accounts"
BEFORE DELETE
ON "accounts"
FOR EACH ROW
EXECUTE PROCEDURE "ondelete_accounts"();

CREATE FUNCTION "ondelete_guilds"()
RETURNS TRIGGER
AS $$
BEGIN
	UPDATE "players" SET "guildnick" = '', "rank_id" = 0 WHERE "rank_id" IN (SELECT "id" FROM "guild_ranks" WHERE "guild_id" = OLD."id");
	DELETE FROM "guild_ranks" WHERE "guild_id" = OLD."id";

	RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER "ondelete_guilds"
BEFORE DELETE
ON "guilds"
FOR EACH ROW
EXECUTE PROCEDURE "ondelete_guilds"();

CREATE FUNCTION "ondelete_players"()
RETURNS TRIGGER
AS $$
BEGIN
	DELETE FROM "bans" WHERE "type" = 2 AND "value" = OLD."id";
	UPDATE "houses" SET "owner" = 0 WHERE "owner" = OLD."id";

	RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER "ondelete_players"
BEFORE DELETE
ON "players"
FOR EACH ROW
EXECUTE PROCEDURE "ondelete_players"();

CREATE FUNCTION "oncreate_guilds"()
RETURNS TRIGGER
AS $$
BEGIN
	INSERT INTO "guild_ranks" ("name", "level", "guild_id") VALUES ('Leader', 3, NEW."id");
	INSERT INTO "guild_ranks" ("name", "level", "guild_id") VALUES ('Vice-Leader', 2, NEW."id");
	INSERT INTO "guild_ranks" ("name", "level", "guild_id") VALUES ('Member', 1, NEW."id");

	RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER "oncreate_guilds"
AFTER INSERT
ON "guilds"
FOR EACH ROW
EXECUTE PROCEDURE "oncreate_guilds"();

CREATE FUNCTION "oncreate_players"()
RETURNS TRIGGER
AS $$
BEGIN
	INSERT INTO "player_skills" ("player_id", "skillid", "value") VALUES (NEW."id", 0, 10);
	INSERT INTO "player_skills" ("player_id", "skillid", "value") VALUES (NEW."id", 1, 10);
	INSERT INTO "player_skills" ("player_id", "skillid", "value") VALUES (NEW."id", 2, 10);
	INSERT INTO "player_skills" ("player_id", "skillid", "value") VALUES (NEW."id", 3, 10);
	INSERT INTO "player_skills" ("player_id", "skillid", "value") VALUES (NEW."id", 4, 10);
	INSERT INTO "player_skills" ("player_id", "skillid", "value") VALUES (NEW."id", 5, 10);
	INSERT INTO "player_skills" ("player_id", "skillid", "value") VALUES (NEW."id", 6, 10);

	RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER "oncreate_players"
AFTER INSERT
ON "players"
FOR EACH ROW
EXECUTE PROCEDURE "oncreate_players"();
