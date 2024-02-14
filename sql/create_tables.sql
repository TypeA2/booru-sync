-- SPDX-License-Identifier: GPL-3.0-or-later
DO $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'tag_category') THEN
        CREATE TYPE tag_category AS ENUM ('general', 'artist', 'copyright', 'character', 'meta');
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'post_rating') THEN
        CREATE TYPE post_rating AS ENUM ('g', 's', 'q', 'e');
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'pool_category') THEN
        CREATE TYPE pool_category AS ENUM ('series', 'collection');
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'asset_status') THEN
        CREATE TYPE asset_status AS ENUM ('processing', 'active', 'deleted', 'expunged', 'failed');
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'file_type') THEN
        CREATE TYPE file_type AS ENUM ('jpg', 'png', 'gif', 'webp', 'avif', 'mp4', 'webm', 'swf', 'zip');
    END IF;
END$$;

CREATE TABLE IF NOT EXISTS metadata (
    key TEXT PRIMARY KEY,
    data JSONB NOT NULL
);

CREATE TABLE IF NOT EXISTS tags (
    id            INTEGER      PRIMARY KEY,
    name          TEXT         NOT NULL UNIQUE,
    post_count    INTEGER      NOT NULL,
    category      tag_category NOT NULL,
    is_deprecated BOOLEAN      NOT NULL,
    created_at	  TIMESTAMP    NOT NULL,
    updated_at    TIMESTAMP    NOT NULL
);

CREATE TABLE IF NOT EXISTS tag_versions (
    id                  INTEGER      PRIMARY KEY,
    tag_id              INTEGER,
    name                TEXT         NOT NULL,
    updater_id          INTEGER,  -- Null on first change/tag creation
    previous_version_id INTEGER      REFERENCES tag_versions(id), -- Null when first version
    version             INTEGER      NOT NULL,
    category            tag_category NOT NULL,
    is_deprecated       BOOLEAN      NOT NULL,
    created_at          TIMESTAMP    NOT NULL,
    updated_at          TIMESTAMP    NOT NULL
);

CREATE TABLE IF NOT EXISTS media_assets (
    id           INTEGER         PRIMARY KEY,
    md5          TEXT            NOT NULL,
    file_ext     file_type       NOT NULL,
    file_size    BIGINT          NOT NULL,
    image_width  INTEGER         NOT NULL,
    image_height INTEGER         NOT NULL,
    duration     REAL,        -- Null for non-video
    pixel_hash   TEXT            NOT NULL,
    status       asset_status    NOT NULL,
    file_key     TEXT            NOT NULL,
    is_public    BOOLEAN         NOT NULL,
    created_at   TIMESTAMP       NOT NULL,
    updated_at   TIMESTAMP       NOT NULL
);

CREATE TABLE IF NOT EXISTS media_asset_variants (
    asset_id INTEGER   NOT NULL REFERENCES media_assets(id),
    type     TEXT      NOT NULL,
    width    INTEGER   NOT NULL,
    height   INTEGER   NOT NULL,
    file_ext file_type NOT NULL,
    PRIMARY KEY (asset_id, type)
);

CREATE TABLE IF NOT EXISTS posts (
    id           INTEGER     PRIMARY KEY,
    uploader_id  INTEGER     NOT NULL,
    approver_id  INTEGER, -- Null when queue bypassed or deleted
    tags         INTEGER[]   NOT NULL, -- Array of tag IDs
    rating       post_rating NOT NULL,
    parent_id    INTEGER, -- Null for parent-less
    source       TEXT,    -- Null for empty source (should save space)
    media_asset  INTEGER     NOT NULL,
    fav_count    INTEGER     NOT NULL,
    has_children BOOLEAN     NOT NULL,
    up_score     INTEGER     NOT NULL,
    down_score   INTEGER     NOT NULL,
    is_pending   BOOLEAN     NOT NULL,
    is_flagged   BOOLEAN     NOT NULL,
    is_deleted   BOOLEAN     NOT NULL,
    is_banned    BOOLEAN     NOT NULL,
    pixiv_id     INTEGER, -- Null for non-Pixiv posts
    bit_flags    INTEGER     NOT NULL,
    last_comment TIMESTAMP, -- Null when none
    last_bump    TIMESTAMP,
    last_note    TIMESTAMP,
    created_at   TIMESTAMP   NOT NULL,
    updated_at   TIMESTAMP   NOT NULL
);

CREATE TABLE IF NOT EXISTS post_versions (
    id           INTEGER   PRIMARY KEY,
    post_id      INTEGER   NOT NULL,
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
    post_id    INTEGER   NOT NULL,
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
    posts       INTEGER[]     NOT NULL
);

-- Indices
-- See also: https://github.com/danbooru/danbooru/blob/master/db/structure.sql

CREATE UNIQUE INDEX IF NOT EXISTS index_tags_on_name ON tags USING btree (name);
CREATE INDEX IF NOT EXISTS index_tags_on_post_count ON tags USING btree (post_count);

CREATE INDEX IF NOT EXISTS index_media_assets_on_md5 ON media_assets USING btree (md5);
CREATE INDEX IF NOT EXISTS index_media_assets_on_pixel_hash ON media_assets USING btree (pixel_hash);

CREATE INDEX IF NOT EXISTS index_posts_on_approver_id ON posts USING btree (approver_id) WHERE (approver_id IS NOT NULL);
CREATE INDEX IF NOT EXISTS index_posts_on_created_at ON posts USING btree (created_at);
CREATE INDEX IF NOT EXISTS index_posts_on_is_deleted ON posts USING btree (is_deleted) WHERE (is_deleted = true);
CREATE INDEX IF NOT EXISTS index_posts_on_is_flagged ON posts USING btree (is_flagged) WHERE (is_flagged = true);
CREATE INDEX IF NOT EXISTS index_posts_on_is_pending ON posts USING btree (is_pending) WHERE (is_pending = true);
CREATE INDEX IF NOT EXISTS index_posts_on_last_bump ON posts USING btree (last_bump DESC NULLS LAST);
CREATE INDEX IF NOT EXISTS index_posts_on_last_note ON posts USING btree (last_note DESC NULLS LAST);
CREATE UNIQUE INDEX IF NOT EXISTS index_posts_on_media_asset ON posts USING btree (media_asset);
CREATE INDEX IF NOT EXISTS index_posts_on_parent_id ON posts USING btree (parent_id) WHERE (parent_id IS NOT NULL);
CREATE INDEX IF NOT EXISTS index_posts_on_pixiv_id ON posts USING btree (pixiv_id) WHERE (pixiv_id IS NOT NULL);
CREATE INDEX IF NOT EXISTS index_posts_on_rating ON posts USING btree (rating) WHERE (rating != 's'::post_rating);
CREATE INDEX IF NOT EXISTS index_posts_on_uploader_id ON posts USING btree (uploader_id) WHERE (uploader_id IS NOT NULL);
CREATE INDEX IF NOT EXISTS index_posts_on_uploader_id_and_created_at ON public.posts USING btree (uploader_id, created_at);

CREATE INDEX IF NOT EXISTS index_post_versions_on_added_tags ON post_versions USING btree (added_tags);
CREATE INDEX IF NOT EXISTS index_post_versions_on_new_parent ON post_versions USING btree (new_parent);
CREATE INDEX IF NOT EXISTS index_post_versions_on_post_id ON post_versions USING btree (post_id);
CREATE INDEX IF NOT EXISTS index_post_versions_on_new_rating ON post_versions USING btree (new_rating);
CREATE INDEX IF NOT EXISTS index_post_versions_on_removed_tags ON post_versions USING btree (removed_tags);
CREATE INDEX IF NOT EXISTS index_post_versions_on_new_source ON post_versions USING btree (new_source);
CREATE INDEX IF NOT EXISTS index_post_versions_on_updated_at ON post_versions USING btree (updated_at);
CREATE INDEX IF NOT EXISTS index_post_versions_on_updater_id ON post_versions USING btree (updater_id);
CREATE INDEX IF NOT EXISTS index_post_versions_on_version ON post_versions USING btree (version);
