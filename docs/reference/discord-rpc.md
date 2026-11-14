> ## Documentation Index
> Fetch the complete documentation index at: https://docs.discord.com/llms.txt
> Use this file to discover all available pages before exploring further.

# RPC

> Learn about Discord's RPC server for local application integration.

## RPC over IPC

Discord's RPC server supports IPC (Inter-Process Communication) as transport for native applications and games. This allows high-performance, local communication with the Discord client without requiring network-level overhead.

> [!WARNING]
>
> We recommend using the [Discord Social SDK](https://docs.discord.com/developers/discord-social-sdk/overview.md) for new projects that are looking to integrate Discord's social features into their game.

###### IPC Path

| Platform | Path Format |
| - | - |
| Windows | `\\?\pipe\discord-ipc-{n}` |
| Linux/macOS | `${XDG_RUNTIME_DIR}/discord-ipc-{n}`, `${TMPDIR}/discord-ipc-{n}`, `${TMP}/discord-ipc-{n}`, `${TEMP}/discord-ipc-{n}`, or `/tmp/discord-ipc-{n}` |

On Linux/macOS, Discord resolves the IPC prefix in this order: `XDG_RUNTIME_DIR`, `TMPDIR`, `TMP`, `TEMP`, then `/tmp` as a final fallback.

### Connecting to IPC

To begin a session, the application must open the IPC socket and send a `HANDSHAKE` opcode.

#### Handshake Payload

The payload is a JSON object containing the RPC version and your application's client ID.

| Field | Type | Description |
| - | - | - |
| `v` | `integer` | RPC version |
| `client_id` | `string` | Your application's client ID |

###### Example Handshake

```
[00 00 00 00] // Opcode 0 (Handshake)
[2D 00 00 00] // Length 45
{"v":1,"client_id":"123456789012345678"}
```

Upon success, Discord will respond with a `FRAME` (Opcode `1`) containing the `READY` event.

Once the handshake is complete, all subsequent requests and responses use the `FRAME` opcode. The internal structure of these frames follows the standard [RPC Payload structure](https://docs.discord.com/developers/topics/rpc.md#payloads-payload-structure)

###### Opcodes

| Opcode | Name | Description |
| - | - | - |
| `0` | `HANDSHAKE` | Sent by the client to initiate the connection |
| `1` | `FRAME` | Used for all standard RPC commands and events |
| `2` | `CLOSE` | Sent by either side to close the connection |
| `3` | `PING` | Sent to check if the connection is alive |
| `4` | `PONG` | Response to a `PING` |

> [accordion] RPC over WebSocket (Deprecated)
>
> > [!CAUTION]
> >
> > This is a deprecated way, which is only available for old participants of private beta. It is preferable to use RPC that uses IPC.
>
> All Discord clients have an RPC server running on localhost that allows control over local Discord clients.
>
> ### Connecting to WebSocket
>
> The local RPC server runs on localhost (`127.0.0.1`) and is set up to process WebSocket connections and proxy API requests.
>
> For WebSocket connections, the connection is always `ws://127.0.0.1:PORT/?v=VERSION&client_id=CLIENT_ID&encoding=ENCODING`:
>
> * `CLIENT_ID` is the client ID of the application accessing the RPC Server.
> * `VERSION` is the version of the RPC Server.
> * `PORT` is the port of the RPC Server.
> * `ENCODING` is the type of encoding for this connection to use. `json` and `etf` are supported.
>
> To begin, you'll need to create an app. Head to [your apps](https://discord.com/developers/applications) and click the big plus button. When you create an app on our Developers site, you must specify an "RPC Origin" and "Redirect URI" from which to permit connections and authorizations. **The origin you send when connecting and the redirect uri you send when exchanging an authorization code for an access token must match one of the ones entered on the Developers site.**
>
> When establishing a WebSocket connection, we verify the Origin header on connection to prevent client ID spoofing. You will be instantly disconnected if the Origin does not match.
>
> If you're connecting to the RPC server from within a browser, RPC origins are usually in the form `SCHEME://HOST[:PORT]`, where `SCHEME` is typically https or http, `HOST` is your domain or ip, and `PORT` is the port of the webserver from which the user will be connecting (omitted for ports 80 and 443). For example, `https://discord.com` would be used if the user were connecting from `https://discord.com/some/page/url`.
>
> If you're connecting to the RPC server from within a non-browser application (like a game), you just need to make sure that the origin is sent with the upgrade request when connecting to the WebSocket. For local testing, we recommend testing with an origin like `https://localhost`. For production apps, we recommend setting the origin to your company/game's domain, for example `https://discord.com`.
>
> ### RPC Server Ports
>
> The port range for Discord's local RPC server is \[6463, 6472]. Since the RPC server runs locally, there's a chance it might not be able to obtain its preferred port when it tries to bind to one. For this reason, the local RPC server will pick one port out of a range of these 10 ports, trying sequentially until it can bind to one. When implementing your client, you should perform the same sequential checking to find the correct port to connect to.
>
> ###### RPC Versions
>
> | Version | Out of Service |
> | - | - |
> | 1 | no |

## Restrictions

For connections to the RPC server, a [list of approved testers](https://docs.discord.com/developers/topics/rpc.md#authorize) is used to restrict access while you're still developing. You can invite up to 50 people.

For applications/games not approved, we limit you to creating 10 guilds and 10 channels. This limit is raised to virtually unlimited after approval.

## Payloads

###### Payload Structure

| Field | Type | Description | Present |
| - | - | - | - |
| cmd | enum | [payload command](https://docs.discord.com/developers/topics/rpc.md#commands-and-events-rpc-commands) | Always |
| nonce | string | unique string used once for replies from the server | In responses to commands (not subscribed events) |
| evt | enum | [subscription event](https://docs.discord.com/developers/topics/rpc.md#commands-and-events-rpc-events) | In subscribed events, errors, and (un)subscribing events |
| data | object | event data | In responses from the server |
| args | object | command arguments | In commands sent to the server |

## Authenticating

In order to call any commands over RPC, you must be authenticated or you will receive a code `4006` error response. To begin, call [AUTHORIZE](https://docs.discord.com/developers/topics/rpc.md#authorize):

###### RPC Authorize Example

```json theme={"system"}
{
  "nonce": "f48f6176-4afb-4c03-b1b8-d960861f5216",
  "args": {
    "client_id": "192741864418312192",
    "scopes": ["rpc", "identify"]
  },
  "cmd": "AUTHORIZE"
}
```

The user will then be prompted to authorize your app to access RPC on Discord. The `AUTHORIZE` command returns a `code` that you can exchange with a POST to `https://discord.com/api/oauth2/token` containing the [standard OAuth2 body parameters](https://tools.ietf.org/html/rfc6749#section-4.1.3) for the token exchange. The token endpoint on our API will return an `access_token` that can be sent with [AUTHENTICATE](https://docs.discord.com/developers/topics/rpc.md#authenticate):

###### RPC Authenticate Example

```json theme={"system"}
{
  "nonce": "5bb10a43-1fdc-4391-9512-0c8f4aa203d4",
  "args": {
    "access_token": "CZhtkLDpNYXgPH9Ml6shqh2OwykChw"
  },
  "cmd": "AUTHENTICATE"
}
```

You can now call RPC commands on behalf of the authorized user!

## Commands and Events

Commands are requests made to the RPC socket by a client.

> [!WARNING]
>
> The structure of responses from the RPC server is similar to the HTTP API but may not be exactly equivalent.

###### RPC Commands

| Name | Description |
| - | - |
| DISPATCH | event dispatch |
| [AUTHORIZE](https://docs.discord.com/developers/topics/rpc.md#authorize) | used to authorize a new client with your app |
| [AUTHENTICATE](https://docs.discord.com/developers/topics/rpc.md#authenticate) | used to authenticate an existing client with your app |
| [GET\_GUILD](https://docs.discord.com/developers/topics/rpc.md#getguild) | used to retrieve guild information from the client |
| [GET\_GUILDS](https://docs.discord.com/developers/topics/rpc.md#getguilds) | used to retrieve a list of guilds from the client |
| [GET\_CHANNEL](https://docs.discord.com/developers/topics/rpc.md#getchannel) | used to retrieve channel information from the client |
| [GET\_CHANNELS](https://docs.discord.com/developers/topics/rpc.md#getchannels) | used to retrieve a list of channels for a guild from the client |
| [SUBSCRIBE](https://docs.discord.com/developers/topics/rpc.md#subscribe) | used to subscribe to an RPC event |
| [UNSUBSCRIBE](https://docs.discord.com/developers/topics/rpc.md#unsubscribe) | used to unsubscribe from an RPC event |
| [SET\_USER\_VOICE\_SETTINGS](https://docs.discord.com/developers/topics/rpc.md#setuservoicesettings) | used to change voice settings of users in voice channels |
| [SELECT\_VOICE\_CHANNEL](https://docs.discord.com/developers/topics/rpc.md#selectvoicechannel) | used to join or leave a voice channel, group dm, or dm |
| [GET\_SELECTED\_VOICE\_CHANNEL](https://docs.discord.com/developers/topics/rpc.md#getselectedvoicechannel) | used to get the current voice channel the client is in |
| [SELECT\_TEXT\_CHANNEL](https://docs.discord.com/developers/topics/rpc.md#selecttextchannel) | used to join or leave a text channel, group dm, or dm |
| [GET\_VOICE\_SETTINGS](https://docs.discord.com/developers/topics/rpc.md#getvoicesettings) | used to retrieve the client's voice settings |
| [SET\_VOICE\_SETTINGS](https://docs.discord.com/developers/topics/rpc.md#setvoicesettings) | used to set the client's voice settings |
| [SET\_CERTIFIED\_DEVICES](https://docs.discord.com/developers/topics/rpc.md#setcertifieddevices) | used to send info about certified hardware devices |
| [SET\_ACTIVITY](https://docs.discord.com/developers/topics/rpc.md#setactivity) | used to update a user's Rich Presence |
| [SEND\_ACTIVITY\_JOIN\_INVITE](https://docs.discord.com/developers/topics/rpc.md#sendactivityjoininvite) | used to consent to a Rich Presence Ask to Join request |
| [CLOSE\_ACTIVITY\_REQUEST](https://docs.discord.com/developers/topics/rpc.md#closeactivityrequest) | used to reject a Rich Presence Ask to Join request |

Events are payloads sent over the socket to a client that correspond to events in Discord.

###### RPC Events

| Name | Description |
| - | - |
| [READY](https://docs.discord.com/developers/topics/rpc.md#ready-ready-dispatch-data-structure) | non-subscription event sent immediately after connecting, contains server information |
| [ERROR](https://docs.discord.com/developers/topics/rpc.md#error-error-data-structure) | non-subscription event sent when there is an error, including command responses |
| [CURRENT\_USER\_UPDATE](https://docs.discord.com/developers/topics/rpc.md#currentuserupdate) | sent when the local user's data (avatar, username, etc.) changes |
| [RELATIONSHIP\_UPDATE](https://docs.discord.com/developers/topics/rpc.md#relationshipupdate) | sent when a relationship (friend, block, etc.) is added or removed |
| [GUILD\_STATUS](https://docs.discord.com/developers/topics/rpc.md#guildstatus) | sent when a subscribed server's state changes |
| [GUILD\_CREATE](https://docs.discord.com/developers/topics/rpc.md#guildcreate) | sent when a guild is created/joined on the client |
| [CHANNEL\_CREATE](https://docs.discord.com/developers/topics/rpc.md#channelcreate) | sent when a channel is created/joined on the client |
| [VOICE\_CHANNEL\_SELECT](https://docs.discord.com/developers/topics/rpc.md#voicechannelselect) | sent when the client joins a voice channel |
| [VOICE\_STATE\_CREATE](https://docs.discord.com/developers/topics/rpc.md#voicestatecreate/voicestateupdate/voicestatedelete) | sent when a user joins a subscribed voice channel |
| [VOICE\_STATE\_UPDATE](https://docs.discord.com/developers/topics/rpc.md#voicestatecreate/voicestateupdate/voicestatedelete) | sent when a user's voice state changes in a subscribed voice channel (mute, volume, etc.) |
| [VOICE\_STATE\_DELETE](https://docs.discord.com/developers/topics/rpc.md#voicestatecreate/voicestateupdate/voicestatedelete) | sent when a user parts a subscribed voice channel |
| [VOICE\_SETTINGS\_UPDATE](https://docs.discord.com/developers/topics/rpc.md#voicesettingsupdate) | sent when the client's voice settings update |
| [VOICE\_CONNECTION\_STATUS](https://docs.discord.com/developers/topics/rpc.md#voiceconnectionstatus) | sent when the client's voice connection status changes |
| [SPEAKING\_START](https://docs.discord.com/developers/topics/rpc.md#speakingstart/speakingstop) | sent when a user in a subscribed voice channel speaks |
| [SPEAKING\_STOP](https://docs.discord.com/developers/topics/rpc.md#speakingstart/speakingstop) | sent when a user in a subscribed voice channel stops speaking |
| [MESSAGE\_CREATE](https://docs.discord.com/developers/topics/rpc.md#messagecreate/messageupdate/messagedelete) | sent when a message is created in a subscribed text channel |
| [MESSAGE\_UPDATE](https://docs.discord.com/developers/topics/rpc.md#messagecreate/messageupdate/messagedelete) | sent when a message is updated in a subscribed text channel |
| [MESSAGE\_DELETE](https://docs.discord.com/developers/topics/rpc.md#messagecreate/messageupdate/messagedelete) | sent when a message is deleted in a subscribed text channel |
| [NOTIFICATION\_CREATE](https://docs.discord.com/developers/topics/rpc.md#notificationcreate) | sent when the client receives a notification (mention or new message in eligible channels) |
| [ACTIVITY\_JOIN](https://docs.discord.com/developers/topics/rpc.md#activityjoin) | sent when the user clicks a Rich Presence join invite in chat to join a game |
| [ACTIVITY\_SPECTATE](https://docs.discord.com/developers/topics/rpc.md#activityspectate) | sent when the user clicks a Rich Presence spectate invite in chat to spectate a game |
| [ACTIVITY\_JOIN\_REQUEST](https://docs.discord.com/developers/topics/rpc.md#activityjoinrequest) | sent when the user receives a Rich Presence Ask to Join request |
| [ACTIVITY\_INVITE](https://docs.discord.com/developers/topics/rpc.md#activityinvite) | sent when the user receives an activity invitation |
| [ENTITLEMENT\_CREATE](https://docs.discord.com/developers/topics/rpc.md#entitlementcreate) | sent when a user purchases or receives a new entitlement (SKU/Game) |
| [ENTITLEMENT\_DELETE](https://docs.discord.com/developers/topics/rpc.md#entitlementdelete) | sent when an entitlement is removed |

#### AUTHORIZE

Used to authenticate a new client with your app. By default this pops up a modal in-app that asks the user to authorize access to your app.

**We currently do not allow access to RPC for unapproved apps without being on the game's list of testers**. We grant 50 testing spots, which should be ample for development. After approval, this restriction is removed and your app will be accessible to anyone.

We also have an RPC token system to bypass the user authorization modal. This is usable by approved games as well as by users on a game's list of testers, and also disallows use of the `messages.read` scope. If you have been granted access, you can send a POST request to `https://discord.com/api/oauth2/token/rpc` with your application's `client_id` and `client_secret` in the body (sent as a url-encoded body, **not JSON**). You can then pass the returned `rpc_token` value to the `rpc_token` field in your RPC authorize request (documented below).

###### Authorize Argument Structure

| Field | Type | Description |
| - | - | - |
| scopes | array of [OAuth2 scopes](https://docs.discord.com/developers/topics/oauth2.md#shared-resources-oauth2-scopes) | scopes to authorize |
| client\_id | string | OAuth2 application id |
| rpc\_token | string | one-time use RPC token |
| username | string | username to create a guest account with if the user does not have Discord |

###### Authorize Response Structure

| Field | Type | Description |
| - | - | - |
| code | string | OAuth2 authorization code |

###### Example Authorize Command Payload

```json theme={"system"}
{
  "nonce": "f48f6176-4afb-4c03-b1b8-d960861f5216",
  "args": {
    "client_id": "192741864418312192",
    "scopes": ["rpc", "identify"]
  },
  "cmd": "AUTHORIZE"
}
```

###### Example Authorize Response Payload

```json theme={"system"}
{
  "cmd": "AUTHORIZE",
  "data": {
    "code": "O62Q9JzFe8BEOUzIfsAndOjNd2V4sJ"
  },
  "nonce": "f48f6176-4afb-4c03-b1b8-d960861f5216"
}
```

#### AUTHENTICATE

Used to authenticate an existing client with your app.

###### Authenticate Argument Structure

| Field | Type | Description |
| - | - | - |
| access\_token | string | OAuth2 access token |

###### Authenticate Response Structure

| Field | Type | Description |
| - | - | - |
| user | partial [user](https://docs.discord.com/developers/resources/user.md#user-object) object | the authed user |
| scopes | array of [OAuth2 scopes](https://docs.discord.com/developers/topics/oauth2.md#shared-resources-oauth2-scopes) | authorized scopes |
| expires | date | expiration date of OAuth2 token |
| application | [OAuth2 application](https://docs.discord.com/developers/topics/rpc.md#authenticate-oauth2-application-structure) object | application the user authorized |

###### OAuth2 Application Structure

| Field | Type | Description |
| - | - | - |
| description | string | application description |
| icon | string | hash of the icon |
| id | snowflake | application client id |
| rpc\_origins | array of strings | array of rpc origin urls |
| name | string | application name |

###### Example Authenticate Command Payload

```json theme={"system"}
{
  "nonce": "5bb10a43-1fdc-4391-9512-0c8f4aa203d4",
  "args": {
    "access_token": "CZhtkLDpNYXgPH9Ml6shqh2OwykChw"
  },
  "cmd": "AUTHENTICATE"
}
```

###### Example Authenticate Response Payload

```json theme={"system"}
{
  "cmd": "AUTHENTICATE",
  "data": {
    "application": {
      "description": "test app description",
      "icon": "d6b51c21c48482d5b64aa4832d92fe14",
      "id": "192741864418312192",
      "rpc_origins": ["http://localhost:3344"],
      "name": "test app"
    },
    "expires": "2017-06-29T19:09:52.361000+00:00",
    "user": {
      "username": "test user",
      "discriminator": "7479",
      "id": "190320984123768832",
      "avatar": "b004ec1740a63ca06ae2e14c5cee11f3"
    },
    "scopes": ["rpc", "identify"]
  },
  "nonce": "5bb10a43-1fdc-4391-9512-0c8f4aa203d4"
}
```

#### GET\_GUILDS

Used to get a list of guilds the client is in.

###### Get Guilds Response Structure

| Field | Type | Description |
| - | - | - |
| guilds | array of partial [guild](https://docs.discord.com/developers/resources/guild.md#guild-object) objects | the guilds the user is in |

###### Example Get Guilds Command Payload

```json theme={"system"}
{
  "nonce": "e16fcbed-8bfa-4fd4-ba09-73b72e809833",
  "args": {},
  "cmd": "GET_GUILDS"
}
```

###### Example Get Guilds Response Payload

```json theme={"system"}
{
  "cmd": "GET_GUILDS",
  "data": {
    "guilds": [
      {
        "id": "199737254929760256",
        "name": "test"
      }
    ]
  },
  "nonce": "e16fcbed-8bfa-4fd4-ba09-73b72e809833"
}
```

#### GET\_GUILD

Used to get a guild the client is in.

###### Get Guild Argument Structure

| Field | Type | Description |
| - | - | - |
| guild\_id | string | id of the guild to get |
| timeout | integer | asynchronously get guild with time to wait before timing out |

###### Get Guild Response Structure

| Field | Type | Description |
| - | - | - |
| id | string | guild id |
| name | string | guild name |
| icon\_url | string | guild icon url |
| members | array of [guild member](https://docs.discord.com/developers/resources/guild.md#guild-member-object) objects | members of the guild (deprecated; always empty array) |

###### Example Get Guild Command Payload

```json theme={"system"}
{
  "nonce": "9524922c-3d32-413a-bdaa-0804f4332588",
  "args": {
    "guild_id": "199737254929760256"
  },
  "cmd": "GET_GUILD"
}
```

###### Example Get Guild Response Payload

```json theme={"system"}
{
  "cmd": "GET_GUILD",
  "data": {
    "id": "199737254929760256",
    "name": "test",
    "icon_url": null,
    "members": []
  },
  "nonce": "9524922c-3d32-413a-bdaa-0804f4332588"
}
```

#### GET\_CHANNEL

Used to get a channel the client is in.

###### Get Channel Argument Structure

| Field | Type | Description |
| - | - | - |
| channel\_id | string | id of the channel to get |

###### Get Channel Response Structure

| Field | Type | Description |
| - | - | - |
| id | string | channel id |
| guild\_id | string | channel's guild id |
| name | string | channel name |
| type | integer | channel type (guild text: 0, guild voice: 2, dm: 1, group dm: 3) |
| topic | string | (text) channel topic |
| bitrate | integer | (voice) bitrate of voice channel |
| user\_limit | integer | (voice) user limit of voice channel (0 for none) |
| position | integer | position of channel in channel list |
| voice\_states | array of [voice state](https://docs.discord.com/developers/resources/voice.md#voice-state-object) objects | (voice) channel's voice states |
| messages | array of [message](https://docs.discord.com/developers/resources/message.md#message-object) objects | (text) channel's messages |

###### Example Get Channel Command Payload

```json theme={"system"}
{
  "nonce": "f682697e-d257-4a17-ac0a-7e4b84e66663",
  "args": {
    "channel_id": "199737254929760257"
  },
  "cmd": "GET_CHANNEL"
}
```

###### Example Get Channel Response Payload

```json theme={"system"}
{
  "cmd": "GET_CHANNEL",
  "data": {
    "id": "199737254929760257",
    "name": "General",
    "type": 2,
    "bitrate": 64000,
    "user_limit": 0,
    "guild_id": "199737254929760256",
    "position": 0,
    "voice_states": [
      {
        "voice_state": {
          "mute": false,
          "deaf": false,
          "self_mute": false,
          "self_deaf": false,
          "suppress": false
        },
        "user": {
          "id": "190320984123768832",
          "username": "test 2",
          "discriminator": "7479",
          "avatar": "b004ec1740a63ca06ae2e14c5cee11f3",
          "bot": false
        },
        "nick": "test user 2",
        "volume": 110,
        "mute": false,
        "pan": {
          "left": 1.0,
          "right": 1.0
        }
      }
    ]
  },
  "nonce": "f682697e-d257-4a17-ac0a-7e4b84e66663"
}
```

#### GET\_CHANNELS

Used to get a guild's channels the client is in.

###### Get Channels Argument Structure

| Field | Type | Description |
| - | - | - |
| guild\_id | string | id of the guild to get channels for |

###### Get Channels Response Structure

| Field | Type | Description |
| - | - | - |
| channels | array of partial [channel](https://docs.discord.com/developers/resources/channel.md#channel-object) objects | guild channels the user is in |

###### Example Get Channels Command Payload

```json theme={"system"}
{
  "nonce": "0dee7bd4-8f62-4ecc-9e0f-1b1839a4fa93",
  "args": {
    "guild_id": "199737254929760256"
  },
  "cmd": "GET_CHANNELS"
}
```

###### Example Get Channels Response Payload

```json theme={"system"}
{
  "cmd": "GET_CHANNELS",
  "data": {
    "channels": [
      {
        "id": "199737254929760256",
        "name": "general",
        "type": 0
      },
      {
        "id": "199737254929760257",
        "name": "General",
        "type": 2
      }
    ]
  },
  "nonce": "0dee7bd4-8f62-4ecc-9e0f-1b1839a4fa93"
}
```

#### SET\_USER\_VOICE\_SETTINGS

Used to change voice settings of users in voice channels

###### Set User Voice Settings Argument and Response Structure

| Field | Type | Description |
| - | - | - |
| user\_id | string | user id |
| pan? | [pan](https://docs.discord.com/developers/topics/rpc.md#setuservoicesettings-pan-object) object | set the pan of the user |
| volume? | integer | set the volume of user (defaults to 100, min 0, max 200) |
| mute? | boolean | set the mute state of the user |

> [!NOTE]
>
> In the current release, we only support a single modifier of voice settings at a time over RPC. If an app changes voice settings, it will lock voice settings so that other apps connected simultaneously lose the ability to change voice settings. Settings reset to what they were before being changed after the controlling app disconnects. When an app that has previously set voice settings connects, the client will swap to that app's configured voice settings and lock voice settings again. This is a temporary situation that will be changed in the future.

###### Pan Object

| Field | Type | Description |
| - | - | - |
| left | float | left pan of user (min: 0.0, max: 1.0) |
| right | float | right pan of user (min: 0.0, max: 1.0) |

###### Example Set User Voice Settings Command Payload

```json theme={"system"}
{
  "nonce": "eafc8152-2248-4478-9827-8457b7900cb4",
  "args": {
    "user_id": "192731515703001088",
    "pan": {
      "left": 1.0,
      "right": 1.0
    },
    "volume": 120,
    "mute": false
  },
  "cmd": "SET_USER_VOICE_SETTINGS"
}
```

###### Example Set User Voice Settings Response Payload

```json theme={"system"}
{
  "cmd": "SET_USER_VOICE_SETTINGS",
  "data": {
    "user_id": "192731515703001088",
    "pan": {
      "left": 1.0,
      "right": 1.0
    },
    "volume": 120,
    "mute": false
  },
  "nonce": "eafc8152-2248-4478-9827-8457b7900cb4"
}
```

#### SELECT\_VOICE\_CHANNEL

Used to join and leave voice channels, group dms, or dms. Returns the [Get Channel](https://docs.discord.com/developers/topics/rpc.md#getchannel) response, `null` if none.

###### Select Voice Channel Argument Structure

| Field | Type | Description |
| - | - | - |
| channel\_id | string | channel id to join (or `null` to leave) |
| timeout | integer | asynchronously join channel with time to wait before timing out |
| force | boolean | forces a user to join a voice channel |
| navigate | boolean | after joining the voice channel, navigate to it in the client |

> [!WARNING]
>
> When trying to join the user to a voice channel, you will receive a `5003` error coded response if the user is already in a voice channel. The `force` parameter should only be specified in response to the case where a user is already in a voice channel and they have **approved** to be moved by your app to a new voice channel.

###### Example Select Voice Channel Command Payload

```json theme={"system"}
{
  "nonce": "5d9df76d-6408-46a1-9368-33dca74fa423",
  "args": {
    "channel_id": "199737254929760257"
  },
  "cmd": "SELECT_VOICE_CHANNEL"
}
```

###### Example Select Voice Channel Response Payload

```json theme={"system"}
{
  "cmd": "SELECT_VOICE_CHANNEL",
  "data": {
    "id": "199737254929760257",
    "name": "General",
    "type": 2,
    "bitrate": 64000,
    "user_limit": 0,
    "guild_id": "199737254929760256",
    "position": 0,
    "voice_states": [
      {
        "voice_state": {
          "mute": false,
          "deaf": false,
          "self_mute": false,
          "self_deaf": false,
          "suppress": false
        },
        "user": {
          "id": "190320984123768832",
          "username": "test 2",
          "discriminator": "7479",
          "avatar": "b004ec1740a63ca06ae2e14c5cee11f3",
          "bot": false
        },
        "nick": "test user 2",
        "mute": false,
        "volume": 110,
        "pan": {
          "left": 1.0,
          "right": 1.0
        }
      }
    ]
  },
  "nonce": "5d9df76d-6408-46a1-9368-33dca74fa423"
}
```

#### GET\_SELECTED\_VOICE\_CHANNEL

Used to get the client's current voice channel. There are no arguments for this command. Returns the [Get Channel](https://docs.discord.com/developers/topics/rpc.md#getchannel) response, or `null` if none.

#### SELECT\_TEXT\_CHANNEL

Used to join and leave text channels, group dms, or dms. Returns the [Get Channel](https://docs.discord.com/developers/topics/rpc.md#getchannel) response, or `null` if none.

###### Select Text Channel Argument Structure

| Field | Type | Description |
| - | - | - |
| channel\_id | string | channel id to join (or `null` to leave) |
| timeout | integer | asynchronously join channel with time to wait before timing out |

#### GET\_VOICE\_SETTINGS

###### Get Voice Settings Response Structure

| Field | Type | Description |
| - | - | - |
| input | [voice settings input](https://docs.discord.com/developers/topics/rpc.md#getvoicesettings-voice-settings-input-object) object | input settings |
| output | [voice settings output](https://docs.discord.com/developers/topics/rpc.md#getvoicesettings-voice-settings-output-object) object | output settings |
| mode | [voice settings mode](https://docs.discord.com/developers/topics/rpc.md#getvoicesettings-voice-settings-mode-object) object | voice mode settings |
| automatic\_gain\_control | boolean | state of automatic gain control |
| echo\_cancellation | boolean | state of echo cancellation |
| noise\_suppression | boolean | state of noise suppression |
| qos | boolean | state of voice quality of service |
| silence\_warning | boolean | state of silence warning notice |
| deaf | boolean | state of self-deafen |
| mute | boolean | state of self-mute |

###### Voice Settings Input Object

| Field | Type | Description |
| - | - | - |
| device\_id | string | device id |
| volume | float | input voice level (min: 0, max: 100) |
| available\_devices | array of objects | array of *read-only* device objects containing `id` and `name` string keys |

###### Voice Settings Output Object

| Field | Type | Description |
| - | - | - |
| device\_id | string | device id |
| volume | float | output voice level (min: 0, max: 200) |
| available\_devices | array of objects | array of *read-only* device objects containing `id` and `name` string keys |

###### Voice Settings Mode Object

| Field | Type | Description |
| - | - | - |
| type | string | voice setting mode type (can be `PUSH_TO_TALK` or `VOICE_ACTIVITY`) |
| auto\_threshold | boolean | voice activity threshold automatically sets its threshold |
| threshold | float | threshold for voice activity (in dB) (min: -100, max: 0) |
| shortcut | [shortcut key combo](https://docs.discord.com/developers/topics/rpc.md#getvoicesettings-shortcut-key-combo-object) object | shortcut key combos for PTT |
| delay | float | the PTT release delay (in ms) (min: 0, max: 2000) |

###### Shortcut Key Combo Object

| Field | Type | Description |
| - | - | - |
| type | integer | see [key types](https://docs.discord.com/developers/topics/rpc.md#getvoicesettings-key-types) |
| code | integer | key code |
| name | string | key name |

###### Key Types

| Type | Id |
| - | - |
| KEYBOARD\_KEY | 0 |
| MOUSE\_BUTTON | 1 |
| KEYBOARD\_MODIFIER\_KEY | 2 |
| GAMEPAD\_BUTTON | 3 |

###### Example Get Voice Settings Response Payload

```json theme={"system"}
{
  "cmd": "GET_VOICE_SETTINGS",
  "data": {
    "input": {
      "available_devices": [
        {
          "id": "default",
          "name": "Default"
        },
        {
          "id": "Built-in Microphone",
          "name": "Built-in Microphone"
        }
      ],
      "device_id": "default",
      "volume": 49.803921580314636
    },
    "output": {
      "available_devices": [
        {
          "id": "default",
          "name": "Default"
        },
        {
          "id": "Built-in Output",
          "name": "Built-in Output"
        }
      ],
      "device_id": "default",
      "volume": 93.00000071525574
    },
    "mode": {
      "type": "VOICE_ACTIVITY",
      "auto_threshold": true,
      "threshold": -46.92622950819673,
      "shortcut": [{ "type": 0, "code": 12, "name": "i" }],
      "delay": 98.36065573770492
    },
    "automatic_gain_control": false,
    "echo_cancellation": false,
    "noise_suppression": false,
    "qos": false,
    "silence_warning": false,
    "deaf": false,
    "mute": false
  },
  "nonce": "fa07c532-bb03-4f75-8b9a-397f5109afb6"
}
```

#### SET\_VOICE\_SETTINGS

> [!NOTE]
>
> In the current release, we only support a single modifier of voice settings at a time over RPC. If an app changes voice settings, it will lock voice settings so that other apps connected simultaneously lose the ability to change voice settings. Settings reset to what they were before being changed after the controlling app disconnects. When an app that has previously set voice settings connects, the client will swap to that app's configured voice settings and lock voice settings again. This is a temporary situation that will be changed in the future.

When setting voice settings, all fields are optional. Only passed fields are updated.

###### Set Voice Settings Argument and Response Structure

| Field | Type | Description |
| - | - | - |
| input | [voice settings input](https://docs.discord.com/developers/topics/rpc.md#getvoicesettings-voice-settings-input-object) object | input settings |
| output | [voice settings output](https://docs.discord.com/developers/topics/rpc.md#getvoicesettings-voice-settings-output-object) object | output settings |
| mode | [voice settings mode](https://docs.discord.com/developers/topics/rpc.md#getvoicesettings-voice-settings-mode-object) object | voice mode settings |
| automatic\_gain\_control | boolean | state of automatic gain control |
| echo\_cancellation | boolean | state of echo cancellation |
| noise\_suppression | boolean | state of noise suppression |
| qos | boolean | state of voice quality of service |
| silence\_warning | boolean | state of silence warning notice |
| deaf | boolean | state of self-deafen |
| mute | boolean | state of self-mute |

###### Example Set Voice Settings Command Payload

```json theme={"system"}
{
  "nonce": "3d64ed55-ef6e-4bd5-99c9-677533babc22",
  "args": {
    "input": {
      "volume": 90.5
    }
  },
  "cmd": "SET_VOICE_SETTINGS"
}
```

###### Example Set Voice Settings Response Payload

```json theme={"system"}
{
  "cmd": "SET_VOICE_SETTINGS",
  "data": {
    "input": {
      "available_devices": [
        {
          "id": "default",
          "name": "Default"
        },
        {
          "id": "Built-in Microphone",
          "name": "Built-in Microphone"
        }
      ],
      "device_id": "default",
      "volume": 90.5
    },
    "output": {
      "available_devices": [
        {
          "id": "default",
          "name": "Default"
        },
        {
          "id": "Built-in Output",
          "name": "Built-in Output"
        }
      ],
      "device_id": "default",
      "volume": 93.00000071525574
    },
    "mode": {
      "type": "VOICE_ACTIVITY",
      "auto_threshold": true,
      "threshold": -46.92622950819673,
      "shortcut": [{ "type": 0, "code": 12, "name": "i" }],
      "delay": 98.36065573770492
    },
    "automatic_gain_control": false,
    "echo_cancellation": false,
    "noise_suppression": false,
    "qos": false,
    "silence_warning": false,
    "deaf": false,
    "mute": false
  },
  "nonce": "3d64ed55-ef6e-4bd5-99c9-677533babc22"
}
```

#### SUBSCRIBE

Used to subscribe to events. `evt` of the payload should be set to the event being subscribed to. `args` of the payload should be set to the args needed for the event.

###### Subscribe Response Structure

| Field | Type | Description |
| - | - | - |
| evt | string | event name now subscribed to |

###### Example Subscribe Command Payload

```json theme={"system"}
{
  "nonce": "be9a6de3-31d0-4767-a8e9-4818c5690015",
  "args": {
    "guild_id": "199737254929760256"
  },
  "evt": "GUILD_STATUS",
  "cmd": "SUBSCRIBE"
}
```

###### Example Subscribe Response Payload

```json theme={"system"}
{
  "cmd": "SUBSCRIBE",
  "data": {
    "evt": "GUILD_STATUS"
  },
  "nonce": "be9a6de3-31d0-4767-a8e9-4818c5690015"
}
```

#### UNSUBSCRIBE

Used to unsubscribe from events. `evt` of the payload should be set to the event that was subscribed to. `args` of the payload should be set to the args needed for the previously subscribed event.

###### Unsubscribe Response Structure

| Field | Type | Description |
| - | - | - |
| evt | string | event name now unsubscribed from |

###### Example Unsubscribe Command Payload

```json theme={"system"}
{
  "nonce": "647d814a-4cf8-4fbb-948f-898aad24f55b",
  "args": {
    "guild_id": "199737254929760256"
  },
  "evt": "GUILD_STATUS",
  "cmd": "UNSUBSCRIBE"
}
```

###### Example Unsubscribe Response Payload

```json theme={"system"}
{
  "cmd": "UNSUBSCRIBE",
  "data": {
    "evt": "GUILD_STATUS"
  },
  "nonce": "647d814a-4cf8-4fbb-948f-898aad24f55b"
}
```

#### SET\_CERTIFIED\_DEVICES

Used by hardware manufacturers to send information about the current state of their certified devices that are connected to Discord.

###### Set Certified Devices Argument Structure

| Field | Type | Description |
| - | - | - |
| devices | array of [certified device](https://docs.discord.com/developers/topics/rpc.md#setcertifieddevices-device-object) objects | a list of devices for your manufacturer, in order of priority |

###### Device Object

| Field | Type | Description |
| - | - | - |
| type | [device type](https://docs.discord.com/developers/topics/rpc.md#setcertifieddevices-device-type) | the type of device |
| id | string | the device's Windows UUID |
| vendor | [vendor](https://docs.discord.com/developers/topics/rpc.md#setcertifieddevices-vendor-object) object | the hardware vendor |
| model | [model](https://docs.discord.com/developers/topics/rpc.md#setcertifieddevices-model-object) object | the model of the product |
| related | array of strings | UUIDs of related devices |
| echo\_cancellation?\* | boolean | if the device's native echo cancellation is enabled |
| noise\_suppression?\* | boolean | if the device's native noise suppression is enabled |
| automatic\_gain\_control?\* | boolean | if the device's native automatic gain control is enabled |
| hardware\_mute?\* | boolean | if the device is hardware muted |

\*These fields are only applicable for `AUDIO_INPUT` device types

###### Vendor Object

| Field | Type | Description |
| - | - | - |
| name | string | name of the vendor |
| url | string | url for the vendor |

###### Model Object

| Field | Type | Description |
| - | - | - |
| name | string | name of the model |
| url | string | url for the model |

###### Device Type

| Type | Value |
| - | - |
| AUDIO\_INPUT | "audioinput" |
| AUDIO\_OUTPUT | "audiooutput" |
| VIDEO\_INPUT | "videoinput" |

###### Example Set Certified Devices Command Payload

```json theme={"system"}
{
  "nonce": "9b4e9711-97f3-4f35-b047-32c82a51978e",
  "cmd": "SET_CERTIFIED_DEVICES",
  "args": {
    "devices": [
      {
        "type": "audioinput",
        "id": "aafc2003-da0e-42a3-b982-6a17a2812510",
        "vendor": {
          "name": "SteelSeries",
          "url": "https://steelseries.com"
        },
        "model": {
          "name": "Arctis 7",
          "url": "https://steelseries.com/gaming-headsets/arctis-7"
        },
        "related": ["aafc2003-da0e-42a3-b982-6a17a2819999"],
        "echo_cancellation": true,
        "noise_suppression": true,
        "automatic_gain_control": true,
        "hardware_mute": false
      }
    ]
  }
}
```

###### Example Set Certified Devices Response Payload

```json theme={"system"}
{
  "nonce": "9b4e9711-97f3-4f35-b047-32c82a51978e",
  "cmd": "SET_CERTIFIED_DEVICES",
  "data": null,
  "evt": null
}
```

#### SET\_ACTIVITY

Used to update a user's Rich Presence.

###### Set Activity Argument Structure

> [!NOTE]
>
> When using `SET_ACTIVITY`, the `activity` object is limited to a `type` of Playing (`0`), Listening (`2`), Watching (`3`), or Competing (`5`).

| Field | Type | Description |
| - | - | - |
| pid | integer | the application's process id |
| activity | [activity](https://docs.discord.com/developers/events/gateway-events.md#activity-object) object | the rich presence to assign to the user |

###### Example Set Activity Payload

```json theme={"system"}
{
  "cmd": "SET_ACTIVITY",
  "args": {
    "pid": 9999,
    "activity": {
      "state": "In a Group",
      "state_url": "https://example.com/groups/50335231-9d9d-4ebd-873b-984787ee4d1d",
      "details": "Competitive | In a Match",
      "details_url": "https://example.com/matches/42340203-2f25-4534-8ff6-2a6509e81207",
      "timestamps": {
        "start": time(nullptr),
        "end": time(nullptr) + (60 * 5 + 23)
      },
      "assets": {
        "large_image": "numbani_map",
        "large_text": "Numbani",
        "large_url": "https://example.wiki/maps/Numbani",
        "small_image": "pharah_profile",
        "small_text": "Pharah",
        "small_url": "https://example.wiki/characters/Pharah"
      },
      "party": {
        "id": GameEngine.GetPartyId(),
        "size": [3, 6]
      },
      "secrets": {
        "join": "025ed05c71f639de8bfaa0d679d7c94b2fdce12f",
        "spectate": "e7eb30d2ee025ed05c71ea495f770b76454ee4e0",
        "match": "4b2fdce12f639de8bfa7e3591b71a0d679d7c93f"
      },
      "instance": true
    }
  },
  "nonce": "647d814a-4cf8-4fbb-948f-898abd24f55b"
}
```

#### SEND\_ACTIVITY\_JOIN\_INVITE

Used to accept an Ask to Join request.

###### Send Activity Join Invite Argument Structure

| Field | Type | Description |
| - | - | - |
| user\_id | snowflake | the id of the requesting user |

###### Example Send Activity Join Invite Payload

```json theme={"system"}
{
  "nonce": "5dc0c062-98c6-47a0-8922-15aerg126",
  "cmd": "SEND_ACTIVITY_JOIN_INVITE",
  "args": {
    "user_id": "53908232506183680"
  }
}
```

#### CLOSE\_ACTIVITY\_REQUEST

Used to reject an Ask to Join request.

###### Close Activity Request Argument Structure

| Field | Type | Description |
| - | - | - |
| user\_id | snowflake | the id of the requesting user |

###### Example Close Activity Request Payload

```json theme={"system"}
{
  "nonce": "5dc0c062-98c6-47a0-8922-15aerg126",
  "cmd": "CLOSE_ACTIVITY_REQUEST",
  "args": {
    "user_id": "53908232506183680"
  }
}
```

#### READY

###### Ready Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| v | integer | RPC version |
| config | [rpc server configuration](https://docs.discord.com/developers/topics/rpc.md#ready-rpc-server-configuration-object) object | server configuration |
| user | partial [user](https://docs.discord.com/developers/resources/user.md#user-object) object | the user to whom you are connected |

###### RPC Server Configuration Object

| Field | Type | Description |
| - | - | - |
| cdn\_host | string | server's cdn |
| api\_endpoint | string | server's api endpoint |
| environment | string | server's environment |

###### Example Ready Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "v": 1,
    "config": {
      "cdn_host": "cdn.discordapp.com",
      "api_endpoint": "//discord.com/api",
      "environment": "production"
    },
    "user": {
      "id": "53908232506183680",
      "username": "Mason",
      "discriminator": "1337",
      "avatar": null
    }
  },
  "evt": "READY"
}
```

#### ERROR

###### Error Data Structure

| Field | Type | Description |
| - | - | - |
| code | integer | RPC Error Code |
| message | string | Error description |

###### Example Error Payload

```json theme={"system"}
{
  "cmd": "AUTHORIZE",
  "data": {
    "code": 4007,
    "message": "No client id provided"
  },
  "evt": "ERROR",
  "nonce": "5102b6f0-c769-4f37-8cca-25fb0ab22628"
}
```

#### GUILD\_STATUS

###### Guild Status Argument Structure

| Field | Type | Description |
| - | - | - |
| guild\_id | string | id of guild to listen to updates of |

###### Guild Status Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| guild | partial [guild](https://docs.discord.com/developers/resources/guild.md#guild-object) object | guild with requested id |
| online | integer | number of online users in guild (deprecated; always 0) |

###### Example Guild Status Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "guild": {
      "id": "199737254929760256",
      "name": "test",
      "icon_url": null
    },
    "online": 0
  },
  "evt": "GUILD_STATUS"
}
```

#### GUILD\_CREATE

No arguments

###### Guild Create Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| id | string | guild id |
| name | string | name of the guild |

###### Example Guild Create Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "id": "199737254929767562",
    "name": "Test Server"
  },
  "evt": "GUILD_CREATE"
}
```

#### CHANNEL\_CREATE

No arguments

###### Channel Create Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| id | string | channel id |
| name | string | name of the channel |
| type | integer | channel type (guild text: 0, guild voice: 2, dm: 1, group dm: 3) |

###### Example Channel Create Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "id": "199737254929760257",
    "name": "General",
    "type": 0
  },
  "evt": "CHANNEL_CREATE"
}
```

#### VOICE\_CHANNEL\_SELECT

No arguments

###### Voice Channel Select Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| channel\_id | string | id of channel (`null` if none) |
| guild\_id | string | id of guild (`null` if none) |

###### Example Voice Channel Select Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "channel_id": "199737254929760257",
    "guild_id": "199737254929760256"
  },
  "evt": "VOICE_CHANNEL_SELECT"
}
```

#### VOICE\_SETTINGS\_UPDATE

###### Voice Settings Argument Structure

No arguments. Dispatches the [Get Voice Settings](https://docs.discord.com/developers/topics/rpc.md#getvoicesettings) response.

###### Example Voice Settings Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "input": {
      "available_devices": [
        {
          "id": "default",
          "name": "Default"
        },
        {
          "id": "Built-in Microphone",
          "name": "Built-in Microphone"
        }
      ],
      "device_id": "default",
      "volume": 49.803921580314636
    },
    "output": {
      "available_devices": [
        {
          "id": "default",
          "name": "Default"
        },
        {
          "id": "Built-in Output",
          "name": "Built-in Output"
        }
      ],
      "device_id": "default",
      "volume": 93.00000071525574
    },
    "mode": {
      "type": "VOICE_ACTIVITY",
      "auto_threshold": true,
      "threshold": -46.92622950819673,
      "shortcut": [{ "type": 0, "code": 12, "name": "i" }],
      "delay": 98.36065573770492
    },
    "automatic_gain_control": false,
    "echo_cancellation": false,
    "noise_suppression": false,
    "qos": false,
    "silence_warning": false
  },
  "evt": "VOICE_SETTINGS_UPDATE"
}
```

#### VOICE\_STATE\_CREATE/VOICE\_STATE\_UPDATE/VOICE\_STATE\_DELETE

Dispatches channel voice state objects

###### Voice State Argument Structure

| Field | Type | Description |
| - | - | - |
| channel\_id | string | id of channel to listen to updates of |

###### Example Voice State Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "evt": "VOICE_STATE_CREATE",
  "data": {
    "voice_state": {
      "mute": false,
      "deaf": false,
      "self_mute": false,
      "self_deaf": false,
      "suppress": false
    },
    "user": {
      "id": "190320984123768832",
      "username": "test 2",
      "discriminator": "7479",
      "avatar": "b004ec1740a63ca06ae2e14c5cee11f3",
      "bot": false
    },
    "nick": "test user 2",
    "volume": 110,
    "mute": false,
    "pan": {
      "left": 1.0,
      "right": 1.0
    }
  }
}
```

#### VOICE\_CONNECTION\_STATUS

No arguments

###### Voice Connection Status Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| state | string | one of the voice connection states listed below |
| hostname | string | hostname of the connected voice server |
| pings | array of integers | last 20 pings (in ms) |
| average\_ping | integer | average ping (in ms) |
| last\_ping | integer | last ping (in ms) |

###### Voice Connection States

| Field | Description |
| - | - |
| DISCONNECTED | TCP disconnected |
| AWAITING\_ENDPOINT | Waiting for voice endpoint |
| AUTHENTICATING | TCP authenticating |
| CONNECTING | TCP connecting |
| CONNECTED | TCP connected |
| VOICE\_DISCONNECTED | TCP connected, Voice disconnected |
| VOICE\_CONNECTING | TCP connected, Voice connecting |
| VOICE\_CONNECTED | TCP connected, Voice connected |
| NO\_ROUTE | No route to host |
| ICE\_CHECKING | WebRTC ice checking |

###### Example Voice Connection Status Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "evt": "VOICE_CONNECTION_STATUS",
  "data": {
    "state": "VOICE_CONNECTED",
    "hostname": "some-server.discord.gg",
    "pings": [20, 13.37],
    "average_ping": 13.37,
    "last_ping": 20
  }
}
```

#### MESSAGE\_CREATE/MESSAGE\_UPDATE/MESSAGE\_DELETE

Dispatches message objects, with the exception of deletions, which only contains the id in the message object.

###### Message Argument Structure

| Field | Type | Description |
| - | - | - |
| channel\_id | string | id of channel to listen to updates of |

###### Example Message Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "channel_id": "199737254929760256",
    "message": {
      "id": "199743874640379904",
      "blocked": false,
      "content": "test",
      "content_parsed": [
        {
          "content": "test",
          "type": "text"
        }
      ],
      "author_color": "#ffffff",
      "edited_timestamp": null,
      "timestamp": "2016-07-05T04:30:50.776Z",
      "tts": false,
      "mentions": [],
      "mention_roles": [],
      "mention_everyone": false,
      "embeds": [],
      "attachments": [],
      "type": 0,
      "pinned": false,
      "author": {
        "id": "190320984123768832",
        "username": "test user 2",
        "discriminator": "7479",
        "avatar": "b004ec1740a63ca06ae2e14c5cee11f3",
        "bot": false
      }
    }
  },
  "evt": "MESSAGE_CREATE"
}
```

#### SPEAKING\_START/SPEAKING\_STOP

###### Speaking Argument Structure

| Field | Type | Description |
| - | - | - |
| channel\_id | string | id of channel to listen to updates of |

###### Speaking Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| user\_id | string | id of user who started/stopped speaking |

###### Example Speaking Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "user_id": "190320984123768832"
  },
  "evt": "SPEAKING_STOP"
}
```

#### NOTIFICATION\_CREATE

No arguments. This event requires the `rpc.notifications.read` [OAuth2 scope](https://docs.discord.com/developers/topics/oauth2.md#shared-resources-oauth2-scopes).

###### Notification Create Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| channel\_id | string | id of channel where notification occurred |
| message | [message](https://docs.discord.com/developers/resources/message.md#message-object) object | message that generated this notification |
| icon\_url | string | icon url of the notification |
| title | string | title of the notification |
| body | string | body of the notification |

###### Example Notification Create Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "channel_id": "199737254929760256",
    "message": {
      "id": "199743874640379904",
      "blocked": false,
      "content": "test",
      "content_parsed": [
        {
          "content": "test",
          "type": "text"
        }
      ],
      "author_color": "#ffffff",
      "edited_timestamp": null,
      "timestamp": "2016-07-05T04:30:50.776Z",
      "tts": false,
      "mentions": [],
      "mention_roles": [],
      "mention_everyone": false,
      "embeds": [],
      "attachments": [],
      "type": 0,
      "pinned": false,
      "author": {
        "id": "190320984123768832",
        "username": "test user 2",
        "discriminator": "7479",
        "avatar": "b004ec1740a63ca06ae2e14c5cee11f3",
        "bot": false
      }
    },
    "icon_url": "https://cdn.discordapp.com/avatars/155607406007681024/8ab559b8286e48270c04471ae382cd9d.jpg",
    "title": "test_user (#general)",
    "body": "test message"
  },
  "evt": "NOTIFICATION_CREATE"
}
```

#### ACTIVITY\_JOIN

No arguments

###### Activity Join Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| secret | string | the [`join_secret`](https://docs.discord.com/developers/developer-tools/game-sdk#activitysecrets-struct) for the given invite |

###### Example Activity Join Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "secret": "025ed05c71f639de8bfaa0d679d7c94b2fdce12f"
  },
  "evt": "ACTIVITY_JOIN"
}
```

#### ACTIVITY\_SPECTATE

No arguments

###### Activity Spectate Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| secret | string | the [`spectate_secret`](https://docs.discord.com/developers/developer-tools/game-sdk#activitysecrets-struct) for the given invite |

###### Example Activity Spectate Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "secret": "e7eb30d2ee025ed05c71ea495f770b76454ee4e0"
  },
  "evt": "ACTIVITY_SPECTATE"
}
```

#### ACTIVITY\_JOIN\_REQUEST

No arguments

###### Activity Join Request Data Structure

| Field | Type | Description |
| - | - | - |
| user | partial [user](https://docs.discord.com/developers/resources/user.md#user-object) object | information about the user requesting to join |

###### Example Activity Join Request Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "user": {
      "id": "53908232506183680",
      "username": "Mason",
      "discriminator": "1337",
      "avatar": "a_bab14f271d565501444b2ca3be944b25"
    }
  },
  "evt": "ACTIVITY_JOIN_REQUEST"
}
```

#### ACTIVITY\_INVITE

No arguments

###### Activity Invite Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| type | integer | invite type; `1` for join |
| user | partial [user](https://docs.discord.com/developers/resources/user.md#user-object) object | user who sent the invite |
| activity | [activity](https://docs.discord.com/developers/events/gateway-events.md#activity-object) object | the activity associated with the invite |
| channel\_id | string | id of the channel the invite was sent in |
| message\_id | string | id of the invite message |

###### Example Activity Invite Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "type": 1,
    "user": {
      "id": "53908232506183680",
      "username": "Mason",
      "discriminator": "1337",
      "avatar": "a_bab14f271d565501444b2ca3be944b25"
    },
    "activity": {
      "application_id": "192741864418312192",
      "name": "My Game",
      "party": {
        "id": "party1234",
        "size": [2, 5]
      }
    },
    "channel_id": "199737254929760256",
    "message_id": "199743874640379904"
  },
  "evt": "ACTIVITY_INVITE"
}
```

#### CURRENT\_USER\_UPDATE

No arguments. Dispatches the current user's profile whenever it changes (avatar, username, etc.).

###### Current User Update Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| id | string | user's id |
| username | string | user's username |
| discriminator | string | user's discriminator |
| global\_name | string | user's display name |
| avatar | string | user's avatar hash |
| avatar\_decoration\_data | object | avatar decoration data, if any (`null` if none) |
| bot | boolean | whether the user is a bot |
| flags | integer | the public [flags](https://docs.discord.com/developers/resources/user.md#user-object-user-flags) on a user's account |
| premium\_type | integer | type of [Nitro subscription](https://docs.discord.com/developers/resources/user.md#user-object-premium-types). Requires `identify.premium` scope. |

###### Example Current User Update Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "id": "53908232506183680",
    "username": "Mason",
    "discriminator": "0",
    "global_name": "Mason",
    "avatar": "a_bab14f271d565501444b2ca3be944b25",
    "avatar_decoration_data": null,
    "bot": false,
    "flags": 64,
    "premium_type": 0
  },
  "evt": "CURRENT_USER_UPDATE"
}
```

#### RELATIONSHIP\_UPDATE

No arguments. Requires the `relationships_read` [OAuth2 scope](https://docs.discord.com/developers/topics/oauth2.md#shared-resources-oauth2-scopes).

Fired when a relationship is added, updated (e.g. presence change), or removed. When a relationship is removed, `type` will be `0` (`NONE`).

###### Relationship Update Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| type | integer | [relationship type](https://docs.discord.com/developers/topics/rpc.md#relationshipupdate-relationship-types) |
| user | partial [user](https://docs.discord.com/developers/resources/user.md#user-object) object | the related user |
| presence | [presence](https://docs.discord.com/developers/topics/rpc.md#relationshipupdate-presence-object) object | the related user's current presence |

###### Relationship Types

| Type | Value | Description |
| - | - | - |
| NONE | 0 | relationship removed |
| FRIEND | 1 | user is a friend |
| BLOCKED | 2 | user is blocked |
| PENDING\_INCOMING | 3 | incoming friend request |
| PENDING\_OUTGOING | 4 | outgoing friend request |
| IMPLICIT | 5 | user is in a mutual guild (not a friend) |

###### Presence Object

| Field | Type | Description |
| - | - | - |
| status | string | user's status (`online`, `idle`, `dnd`, `offline`) |
| activity | [activity](https://docs.discord.com/developers/events/gateway-events.md#activity-object) object | user's current activity for this application (`null` if none) |

###### Example Relationship Update Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "type": 1,
    "user": {
      "id": "190320984123768832",
      "username": "test user 2",
      "discriminator": "0",
      "global_name": "test user 2",
      "avatar": "b004ec1740a63ca06ae2e14c5cee11f3",
      "bot": false,
      "flags": 0,
      "premium_type": 0
    },
    "presence": {
      "status": "online",
      "activity": null
    }
  },
  "evt": "RELATIONSHIP_UPDATE"
}
```

#### ENTITLEMENT\_CREATE

No arguments. Fired when the user acquires a new entitlement for this application.

###### Entitlement Create Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| entitlement | [entitlement](https://docs.discord.com/developers/topics/rpc.md#entitlementcreate-entitlement-object) object | the entitlement that was created |

###### Entitlement Object

| Field | Type | Description |
| - | - | - |
| id | string | entitlement id |
| sku\_id | string | id of the SKU this entitlement is for |
| application\_id | string | id of the application |
| user\_id | string | id of the user that owns the entitlement |
| type | integer | [entitlement type](https://docs.discord.com/developers/topics/rpc.md#entitlementcreate-entitlement-types) |
| deleted | boolean | whether the entitlement has been deleted |
| starts\_at? | ISO8601 | start date of the entitlement |
| ends\_at? | ISO8601 | end date of the entitlement |
| guild\_id? | string | id of the guild the entitlement applies to |
| consumed? | boolean | for consumable entitlements, whether the entitlement has been consumed |

###### Entitlement Types

| Type | Value | Description |
| - | - | - |
| PURCHASE | 1 | purchased by a user |
| PREMIUM\_SUBSCRIPTION | 2 | a Nitro subscription |
| DEVELOPER\_GIFT | 3 | gifted by a developer |
| TEST\_MODE\_PURCHASE | 4 | purchased in test mode |
| FREE\_PURCHASE | 5 | granted for free |
| USER\_GIFT | 6 | gifted by another user |
| PREMIUM\_PURCHASE | 7 | purchased as a premium feature |
| APPLICATION\_SUBSCRIPTION | 8 | an app subscription |

###### Example Entitlement Create Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "entitlement": {
      "id": "1019653849998299136",
      "sku_id": "1019475255913222144",
      "application_id": "192741864418312192",
      "user_id": "53908232506183680",
      "type": 8,
      "deleted": false,
      "starts_at": "2022-09-14T17:00:18.704163+00:00",
      "ends_at": "2022-10-14T17:00:18.704163+00:00"
    }
  },
  "evt": "ENTITLEMENT_CREATE"
}
```

#### ENTITLEMENT\_DELETE

No arguments. Fired when an entitlement for this application is removed. The entitlement object in the payload reflects the state of the entitlement at the time of deletion.

###### Entitlement Delete Dispatch Data Structure

| Field | Type | Description |
| - | - | - |
| entitlement | [entitlement](https://docs.discord.com/developers/topics/rpc.md#entitlementcreate-entitlement-object) object | the entitlement that was deleted |

###### Example Entitlement Delete Dispatch Payload

```json theme={"system"}
{
  "cmd": "DISPATCH",
  "data": {
    "entitlement": {
      "id": "1019653849998299136",
      "sku_id": "1019475255913222144",
      "application_id": "192741864418312192",
      "user_id": "53908232506183680",
      "type": 8,
      "deleted": true,
      "starts_at": "2022-09-14T17:00:18.704163+00:00",
      "ends_at": "2022-10-14T17:00:18.704163+00:00"
    }
  },
  "evt": "ENTITLEMENT_DELETE"
}
```
