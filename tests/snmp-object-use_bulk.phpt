--TEST--
SNMP::$use_bulk selects GETBULK or GETNEXT for walks
--EXTENSIONS--
snmp
--FILE--
<?php
/* Capture the first request PDU sent by walk() on a local UDP socket that
 * never answers, and report its PDU type. */
function walk_pdu_type(int $version, ?bool $use_bulk): string {
    $server = stream_socket_server('udp://127.0.0.1:0', $errno, $errstr, STREAM_SERVER_BIND);
    $port = (int) substr(strrchr(stream_socket_get_name($server, false), ':'), 1);

    $session = new SNMP($version, "127.0.0.1:$port", 'public', 50000, 0);
    if ($use_bulk !== null) {
        $session->use_bulk = $use_bulk;
    }
    @$session->walk('.1.3.6.1.2.1.1');

    $packet = stream_socket_recvfrom($server, 65535);
    fclose($server);

    /* SEQUENCE { INTEGER version, OCTET STRING community, PDU } */
    $pos = 1;
    $len = ord($packet[$pos++]);
    if ($len & 0x80) {
        $pos += $len & 0x7f;
    }
    $pos += 2 + ord($packet[$pos + 1]); /* version */
    $pos += 2 + ord($packet[$pos + 1]); /* community */

    return match (ord($packet[$pos])) {
        0xa1 => 'GETNEXT',
        0xa5 => 'GETBULK',
        default => sprintf('0x%02x', ord($packet[$pos])),
    };
}

$session = new SNMP(SNMP::VERSION_2c, '127.0.0.1', 'public');
var_dump($session->use_bulk);
$session->use_bulk = false;
var_dump($session->use_bulk);

echo "v2c default: ", walk_pdu_type(SNMP::VERSION_2c, null), "\n";
echo "v2c use_bulk=true: ", walk_pdu_type(SNMP::VERSION_2c, true), "\n";
echo "v2c use_bulk=false: ", walk_pdu_type(SNMP::VERSION_2c, false), "\n";
echo "v1 use_bulk=true: ", walk_pdu_type(SNMP::VERSION_1, true), "\n";
?>
--EXPECT--
bool(true)
bool(false)
v2c default: GETBULK
v2c use_bulk=true: GETBULK
v2c use_bulk=false: GETNEXT
v1 use_bulk=true: GETNEXT
