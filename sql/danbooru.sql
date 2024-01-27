-- SPDX-License-Identifier: GPL-3.0-or-later
DO $$
BEGIN
	IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typename = 'tag_category') THEN
		CREATE TYPE tag_category AS ENUM ('general', 'artist', 'copyright', 'character', 'meta');
	END IF;

	IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typename = 'post_rating') THEN
		CREATE TYPE post_rating AS ENUM ('G', 'S', 'Q', 'E');
	END IF;

	IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typename = 'pool_category') THEN
		CREATE TYPE pool_category AS ENUM ('series', 'collection');
	END IF;
END$$;

CREATE TABLE IF NOT EXISTS tags (
	id            INTEGER      PRIMARY KEY,
	name          TEXT         NOT NULL UNIQUE,
	post_count    INTEGER      NOT NULL,
	category      tag_category NOT NULL,
	is_deprecated BOOLEAN      NOT NULL,
	created_at	  TIMESTAMP    NOT NULL,
	updated_at    TIMESTAMP    NOT NULL
);

CREATE TABLE IF NOT EXISTS posts (
	id           INTEGER     PRIMARY KEY,
	uploader_id  INTEGER     NOT NULL,
	approver_id  INTEGER, -- Null when queue bypassed or deleted
	tags         INTEGER[]   NOT NULL, -- Array of tag IDs
	rating       post_rating NOT NULL,
	parent_id    INTEGER, -- Null for parent-less
	source       TEXT,    -- Null for empty source (should save space)
	md5          TEXT,    -- Null for banned posts, etc
	file_ext     TEXT        NOT NULL,
	file_size    BIGINT      NOT NULL,
	image_width  INTEGER     NOT NULL,
	image_height INTEGER     NOT NULL,
	fav_count    INTEGER     NOT NULL,
	has_children BOOLEAN     NOT NULL,
	up_score     INTEGER     NOT NULL,
	down_score   INTEGER     NOT NULL,
	is_deleted   BOOLEAN     NOT NULL,
	is_banned    BOOLEAN     NOT NULL,
	last_comment TIMESTAMP   NOT NULL,
	last_bump    TIMESTAMP   NOT NULL,
	last_note    TIMESTAMP   NOT NULL,
	crEated_at   TIMESTAMP   NOT NULL,
	updated_at   TIMESTAMP   NOT NULL
);

CREATE TABLE IF NOT EXISTS post_versions (
	id           INTEGER   PRIMARY KEY,
	post_id      INTEGER   REFERENCES posts (id),
	updater_id   INTEGER   NOT NULL,
	updated_at   TIMESTAMP NOT NULL,
	version      INTEGER   NOT NULL, -- Not unique, some duplicate versions exist

	-- Null if unchanged
	added_tags   INTEGER[],
	removed_tags INTEGER[],
	new_rating   post_rating,
	new_parent   INTEGER,
	new_source   TEXT
);

CREATE TABLE IF NOT EXISTS comments (
	id         INTEGER   PRIMARY KEY,
	created_at TIMESTAMP NOT NULL,
	updated_at TIMESTAMP NOT NULL,
	post_id    INTEGER   REFERENCES posts (id),
	creator_id INTEGER   NOT NULL,
	body       TEXT      NOT NULL,
	score      INTEGER   NOT NULL,
	is_bump    BOOLEAN   NOT NULL,
	is_deleted BOOLEAN   NOT NULL,
	is_sticky  BOOLEAN   NOT NULL
);

CREATE TABLE IF NOT EXISTS pools (
	id          INTEGER       PRIMARY KEY,
	name        TEXT          NOT NULL,
	created_at  TIMESTAMP     NOT NULL,
	updated_at  TIMESTAMP     NOT NULL,
	description TEXT,         -- Can be empty
	is_active   BOOLEAN       NOT NULL,
	is_deleted  BOOLEAN       NOT NULL,
	category    pool_category NOT NULL,
	posts       INTEGER[]     NOT NULL,
);
