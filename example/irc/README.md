# IRC

<p align="center">
    <img src="../../assets/screenshots/irc.png" alt="irc example">
</p>

A basic IRC client built with stdui. Fill in the connection details and click **Connect** to join a server.

## Slash commands

Type any of these into the message box and press Enter:

| Command           | Description                                                       |
| ----------------- | ----------------------------------------------------------------- |
| `/join <channel>` | Join a channel. The `#` prefix is added automatically if omitted. |
| `/part`           | Leave the current channel.                                        |
| `/leave`          | Alias for `/part`.                                                |
| `/nick <name>`    | Change your nickname.                                             |
| `/me <action>`    | Send a CTCP ACTION message (displayed as `* nick action`).        |
| `/disconnect`     | Close the connection and return to the connect screen.            |
| `/quit`           | Alias for `/disconnect`.                                          |
