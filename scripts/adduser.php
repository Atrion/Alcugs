<?php
// this script was written by MercAngel and adapted by diafero
require 'adduser-config.php';

?>
<html>
<head>
  <title>Alcugs Account Creation</title>
</head>

<body>

<?php
if ($_GET['action'] == 'go') {
  if ($_POST['login'] == '' || $_POST['password'] == '')
    die ("You must enter a username and a password! Click <a ".
         "href=\"javascript:history.back()\">here</a> to go back.");
  if ($_POST['password'] != $_POST['password2'])
    die ("Your passwords don't match! Please go <a ".
         "href=\"javascript:history.back()\">back</a> and re-enter it."); 
  $login = (get_magic_quotes_gpc() ? $_POST['login'] : addslashes($_POST['login']));
  $hashpw = strtoupper(md5(get_magic_quotes_gpc() ? stripslashes($_POST['password']) : $_POST['password']));

  // check name
  if (!preg_match("/^[A-Za-z0-9_\-]{1,100}$/", $login))
    // Spaces are NOT ok - Uru will complain about a syntax error while logging in
    die ("The name you entered is invalid: Valid characters are numbers, letters as well as '_' and '-'. Please go <a ".
         "href=\"javascript:history.back()\">back</a> and re-enter it.");

  mysql_connect($dbhost, $dbuser, $dbpass);
  mysql_select_db($dbname);
  $result = mysql_query("SELECT * FROM accounts WHERE name = '$login'");
  if (mysql_num_rows($result) > 0) {
    echo "An account with that name already exists in the database.  If this ".
      "is not your account, please go <a href=\"javascript:history.back()\">".
      "back</a> and choose another login name.";
  } else {
    mysql_query("INSERT INTO accounts (name, passwd, a_level, guid) ".
		"VALUES ('$login', '$hashpw', '15', UUID())");
    echo "Congratulations, your account has been successfully created!";
  }
  mysql_close();
} else {
?>
<h1>Alcugs Account Creation</h1>
<p>Please select a username and password for logging into this Alcugs Shard.
   Please choose a username and password that you will remember.  If you
   forget your password, it can be reset, but it
   can't be retrieved, since the passwords are encrypted.</p>
<br>
<form action="<?php echo basename($_SERVER['SCRIPT_FILENAME']); ?>?action=go" method="post">
<table border="0">
  <tr>
    <td>Login Name:</td>
    <td><input type="text" name="login" /></td>
  </tr><tr>
    <td>Password:</td>
    <td><input type="password" name="password" /></td>
  </tr><tr>
    <td>Password again:</td>
    <td><input type="password" name="password2" /></td>
  </tr>
</table><br>
<input type="submit" value="Create Account" />
</form>
<? } ?>

</body>
</html>
