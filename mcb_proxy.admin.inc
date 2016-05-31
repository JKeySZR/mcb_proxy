<?php

/**
 * Вспомогательная функция по  фильтрации строк в текстовом поле
 * 
 * 
 * @param type $text
 */
function _mcb_proxy_sites_filter(&$text) {
  $t = explode("\n", $text);
  foreach ($t as $key => $value) {
    // Если есть пустые строки - то уберем их    
    $len = mb_strlen($value);
    if ($len == 1) {
      unset($t[$key]);
      continue;
    }
    // Приводим к нижнему регистру
    $t[$key] = strtolower($value);
    $url = parse_url($value);
  }
  $out = serialize($t);

  return $out;
}

/**
 *  Очищаем URL 
 * 
 * @param type $text
 * @return type
 */
function _mcb_proxy_parse_url(&$str) {
  $url = parse_url($str);
  // В целях нивелирования кривезни рук заполняющих списки будем 
  // отсекать все лишнее и оставляеть только домен
  $domain = '';
  // Если определили домен тогда вернуть его
  if (isset($url['host'])) {
    $domain = $url['host'];
    return $domain;
  }
  if (isset($url['path'])) {
    $t = explode('/', $url['path']);
    $domain = $t[0];
    return $domain;
  }
  $out = 'Neznamo - chto';
  return $domain;
}

/**
 * Вспомогательная функция готовит URL  для regEXP SQUID 
 * 
 * @param type $str
 * @return type
 */
function _mcb_proxy_prepare_regexp(&$str) {
  $ar = unserialize($str);

  $patterns = array();
  $patterns[0] = '/\?/';
  $patterns[1] = '/\./';
  $patterns[2] = '/\+/';
//  $patterns[3] = '/^/';  
  $replacements = array();
  $replacements[0] = '\?';
  $replacements[1] = '\.';
  $replacements[2] = '\+';
//  $replacements[3] = '.';    
  foreach ($ar as $key => $value) {
    $ar[$key] = preg_replace($patterns, $replacements, $value);
  }

  $result = implode("\n", $ar);


  return $result;
}

function _mcb_proxy_getGroupDN_byID($group_id) {
  $group_id = db_select('mcb_proxy_group', 'g')
      ->fields('g', array('ldap_dn'))
      ->condition('id', $group_id)
      ->execute()
      ->fetchCol();
  return $group_id[0];
}



/**
 * Implements hook_forms();
 */
function mcb_proxy_form_user_add($form_id, &$form_state) {


  $form['login'] = array(
    '#type' => 'textfield',
    '#title' => 'Novel login DN',
    '#size' => 40,
    '#maxlength' => 30,
    '#autocomplete_path' => 'mcb/mcb_proxy/admin/user/autocomplete',
    '#description' => '<em>Например cn=bocharov-ms,ou=HQM,o=MCB</em><br>'
    . 'Также работает автопоиск, по мере набора фамилии',
    '#weight' => 10,
  );


  $form['vt'] = array(
    '#type' => 'vertical_tabs',
    '#weight' => 20,
  );

  //drupal_set_title('Я тебя по IP вычислю');
//  $form['user'] = array(
//    '#type' => 'fieldset',
//    '#title' => 'Пользователь',
////    '#attributes' => array('class' => array('f_bss')),
//    '#group' => 'vt',
//  );



  $header = array(
    'eDir?',
    'Group Name',
    'Description',
  );

  // В какую группу запихнуть пользователя
  $form['grp'] = array(
    '#type' => 'fieldset',
    '#title' => 'Группы',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 100,
  );

  $form['grp']['groups'] = array
    (
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => _mcb_proxy_groups_list(),
    '#empty' => 'Группы отсутствуют, необходимо их создать',
  );


  // Account settings.
  $form['sites_sfa'] = array(
    '#type' => 'fieldset',
    '#title' => 'Разрешенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 10,
  );


  $form['sites_sfa']['sfa'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter allow',
    '#rows' => 15,
  );

  $form['sites_sfd'] = array(
    '#type' => 'fieldset',
    '#title' => 'Запрещенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 20,
  );

  $form['sites_sfd']['sfd'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter deny',
    //'#default_value' => _user_mail_text('register_admin_created_body', NULL, array(), FALSE),
    '#rows' => 15,
  );


  $form['submit'] = array(
    '#value' => 'Добавить',
    '#type' => 'submit',
    '#attributes' => array('class' => array('btn find')),
    '#weight' => 500,
  );

  return $form;
}

function mcb_proxy_form_user_add_validate($form_id, &$form_state) {
  $dbg = 6;
  $values = $form_state['values'];
  //--- Смотрим есть ли такой пользователь в мета-информации
  $id = db_select('mcb_proxy_users', 'u')
      ->fields('u', array('id'))
      ->condition('login', $values['login'])
      ->execute()
      ->fetchCol();  
  if (isset($id) && count($id) > 0) {
    form_set_error('login', 'Такой пользователь уже есть в реестре!');
  }
  
}

/**
 * 
 * @param type $form_id
 * @param type $form_state
 */
function mcb_proxy_form_user_add_submit($form_id, &$form_state) {
  $dbg = 0;

  $values = $form_state['values'];
  // Массив полей для записи в таблицу
  $data = array(
    'login' => strtolower($values['login']),
  );

  // Обработка строк из текстового поля
  if (isset($values['sfa']) && $values['sfa'] != '') {
    $sfa = _mcb_proxy_sites_filter($values['sfa']);
    $data['sfa'] = $sfa;
    $data['inet'] = MCB_PROXY_ALLOW_ONLY;
  }
  // Обработка строк из текстового поля
  if (isset($values['sfd']) && $values['sfa'] != '') {
    $sfd = _mcb_proxy_sites_filter($values['sfd']);
    $data['sfd'] = $sfd;

    if ($data['inet'] == MCB_PROXY_ALLOW_ONLY) {
      $data['inet'] = MCB_PROXY_LIMITED;
    }
    else {
      $data['inet'] = MCB_PROXY_DENY_ONLY;
    }
  }

  // Пишем в таблицу данные
  $id = db_insert('mcb_proxy_users')
      ->fields($data)
      ->execute();
  
  // Группы 
  foreach ($values['groups'] as $key => $value) {
    if ((int) $value != 0) {
      $group_dn = _mcb_proxy_getGroupDN_byID($value);      
      mcb_proxy_userAdd($values['login'],$group_dn);
    }
  };  
  
  
  drupal_set_message('Пользователь' . $values['login'] . 'в реестр добавлен.');
}

/**
 *  Форма обновления  данных по пользователю
 * 
 * Implements hook_forms();
 */
function mcb_proxy_form_user_edit($form_id, &$form_state, $id_user) {

  $dbg = 0;

  $query = db_select('mcb_proxy_users', 'u')
      ->fields('u')
      ->condition('id', $id_user)
      ->execute();
  $result = $query->fetchAll();

  $novell_user = current($result);

  $form['user']['login'] = array(
    '#type' => 'textfield',
    '#title' => 'Novel login',
    '#default_value' => $novell_user->login,
    '#size' => 40,
    '#maxlength' => 100,
    '#autocomplete_path' => 'mcb/mcb_proxy/user/autocomplete',
    '#description' => '<em>Например cn=bocharov-ms,ou=HQM,o=MCB</em><br>',
  );


  $form['vt'] = array(
    '#type' => 'vertical_tabs',
  );

  $form['user']['user_id'] = array(
    '#type' => 'hidden',
    '#value' => $id_user,
  );


  $header = array(
    'eDir?',
    'Group Name',
    'Description',
  );

  // В какую группу запихнуть пользователя
  $form['grp'] = array(
    '#type' => 'fieldset',
    '#title' => 'Группы',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 100,
  );

  // Смотрим в каих группах состоит пользователь
  $grp_ids = db_select('mcb_proxy_user2group', 'u2gr')
      ->fields('u2gr', array('group_id'))
      ->condition('user_id', $novell_user->id)
      ->execute()
      ->fetchAll();

  foreach ($grp_ids as $grp) {
    $options[$grp->group_id] = true;
  }
  $dbg = 0;

  $form['grp']['groups'] = array
    (
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => _mcb_proxy_groups_list(),
    '#default_value' => $options, // <- added option
    '#empty' => 'Группы отсутствуют, необходимо их создать',
  );

  // Account settings.
  $form['sites_sfa'] = array(
    '#type' => 'fieldset',
    '#title' => 'Разрешенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 10,
  );

  $form['sites_sfa']['sfa'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter allow',
    '#default_value' => implode("\n", unserialize($novell_user->sfa)),
    //'#default_value' => _user_mail_text('register_admin_created_body', NULL, array(), FALSE),
    '#rows' => 15,
  );

  $form['sites_sfa']['sfa_acl'] = array(
    '#type' => 'textarea',
    '#title' => 'RegExp filter',
    '#default_value' => _mcb_proxy_prepare_regexp($novell_user->sfa),
    //'#default_value' => _user_mail_text('register_admin_created_body', NULL, array(), FALSE),
    '#rows' => 15,
  );



  // Account settings.
  $form['sites_sfd'] = array(
    '#type' => 'fieldset',
    '#title' => 'Запрещенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 20,
  );

  $form['sites_sfd']['sfd'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter deny',
    '#default_value' => implode("\n", unserialize($novell_user->sfd)),
    '#rows' => 15,
  );

  $form['sites_sfd']['sfd_acl'] = array(
    '#type' => 'textarea',
    '#title' => 'RegExp filter',
    '#default_value' => _mcb_proxy_prepare_regexp($novell_user->sfd),
    '#rows' => 15,
  );


  $form['submit'] = array(
    '#value' => 'Обновить',
    '#type' => 'submit',
    '#attributes' => array('class' => array('btn find')),
  );

  return $form;
}

/**
 * Обновление данных по пользователю - нажали кнопку обновить
 * 
 * @param type $form_id
 * @param type $form_state
 */
function mcb_proxy_form_user_edit_submit($form_id, &$form_state) {
  $dbg = 0;
  $values = $form_state['values'];
  // Массив полей для записи в таблицу
  $data = array(
    'login' => strtolower($values['login']),
  );

  // Обработка строк из текстового поля
  if (isset($values['sfa']) && $values['sfa'] != '') {
    $sfa = _mcb_proxy_sites_filter($values['sfa']);
    $data['sfa'] = $sfa;
    $data['inet'] = MCB_PROXY_ALLOW_ONLY;
  }
  // Обработка строк из текстового поля
  if (isset($values['sfd']) && $values['sfa'] != '') {
    $sfd = _mcb_proxy_sites_filter($values['sfd']);
    $data['sfd'] = $sfd;

    if ($data['inet'] == MCB_PROXY_ALLOW_ONLY) {
      $data['inet'] = MCB_PROXY_LIMITED;
    }
    else {
      $data['inet'] = MCB_PROXY_DENY_ONLY;
    }
  }
$dbg = 0;

  $user_id = $values['user_id'];
     
  $u2g = db_select('mcb_proxy_user2group', 'u2g')
      ->fields('u2g')      
      ->condition('user_id', $user_id)
      ->execute()
      ->fetchAllKeyed(0);  
  
  // Группы 
  foreach ($values['groups'] as $key => $value) {
    if (isset($u2g[$key]) && (int) $value == 0) {
      $group_dn = _mcb_proxy_getGroupDN_byID($key);      
      mcb_proxy_userDell($values['login'],$group_dn);
    }
    // Если добавили новую группу, кторой ранее не было
    if (!isset($u2g[$key]) && (int) $value != 0) {
      $group_dn = _mcb_proxy_getGroupDN_byID($value);      
      mcb_proxy_userAdd($values['login'],$group_dn);
    }    
        
  };

  db_update('mcb_proxy_users')
      ->fields($data)
      ->condition('id', $values['user_id'])
      ->execute();
}

/**
 *  Импорт пользователей из формата CSV
 * 
 */
function mcb_proxy_form_user_import() {

  $form['information'] = array(
    '#markup' => '<em>Необходимо скормить csv файл в следующем формате: novell login | novell Context | allow site </em>',
  );

// надо добавить опции csv  разделители экранирование символы

  $form['file'] = array(
    '#type' => 'file',
    '#title' => 'Список пользователей',
    '#description' => 'Выберите файл с расширением csv',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Отправить',
  );
  return $form;
}

/**
 *  Проверяем загруженный файл
 * 
 */
function mcb_proxy_form_user_import_validate() {
  $validators = array(
    'file_validate_extensions' => array('csv'), // Проверка на расширения    
  );
  // Загружаем файл в sites/default/files (по умолчанию)
  if ($file = file_save_upload('file', $validators, 'public://')) {
    $form_state['values']['file'] = $file; // передаём информацию о файле в ф-ю mymodule_form_submit()
  }
  else {
    form_set_error('file', 'Файл не был загружен');
  }
}

/**
 * Form submit callback.
 */
function mcb_proxy_form_user_import_submit($form, &$form_state) {
  $dbg = 0;
  $file = $form_state['values']['file'];
  $file->status = FILE_STATUS_PERMANENT; // Изменяем статус файла на "Постоянный"
  file_save($file); // Сохраняем новый статус
  // Читаем файл, готовим данные для Добавления в БД
  $pusers = array();
  $homepage = file_get_contents('http://www.example.com/');


  drupal_set_message('Картинка загружена');
}

/**
 *  
 * 
 */
function mcb_proxy_page_user_import() {
  $dbg = 0;
  $content = 'VAGA';
  $file = '/opt/drbd/www/domains/it.metcombank.net/SQUID_ARM_USERS_clear.csv';
  $data = file_get_contents($file);
  $csv = explode("\n", $data);

  $user_acl = array();

  // Устанавливает разделитель поля (только один символ).
  $csv_delimiter = ',';
  // Устанавливает символ ограничителя поля (только один символ).
  $csv_enclosure = '"';
  // Устанавливает экранирующий символ (только один символ). По умолчанию равен обратному слешу (\).
  $csv_escape = '';
  foreach ($csv as $row) {
    // Приводим к нижнему регистру
    $row = strtolower($row);
    $d = str_getcsv($row, $csv_delimiter, $csv_enclosure);
    $login = $d[0];

    if ($d[0] == "" || $d[1] == "")
      continue;

    $url = _mcb_proxy_parse_url($d[1]);
    $user_acl[$login][] = $url;
  }
  $dbg = 0;
  // Пишем в БД
  mcb_proxy_user_acl($user_acl);
  return $content;
}

/**
 * 
 * 
 * @param array $data = (
  login = array (
 *      url1,
 *      url2,
 *      ) 
 *  ) 
 * login = bocharov-ms.hqm 
 */
function mcb_proxy_user_acl($data) {

  foreach ($data as $key => $sites) {
    $t = explode('.', $key);
    $login = $t[0];
    $context = $t[1];
    // Массив полей для записи в таблицу
    $data = array(
      'login' => $login,
      'inet' => MCB_PROXY_ALLOW_ONLY,
    );

    $data['sfa'] = serialize($sites);
    // !!! Необходимо проверить есть ли такой пользователь или нет ?  
    // UPDATE OR INSERT in DB
    $exist = db_select('mcb_proxy_users', 'mcb_u')
            ->fields('mcb_u')
            ->condition('login', $login)
            ->execute()->fetchAll();
    if ($exist) {

      db_update('mcb_proxy_users')
          ->fields($data)
          ->condition('id', $exist[0]->id)
          ->execute();
    }
    else {
      // Пишем в таблицу данные
      $id = db_insert('mcb_proxy_users')
          ->fields($data)
          ->execute();
    }
  }
}

/**
 *  Добавление группы во внутреннию систему учета
 * 
 * @return string
 */
function mcb_proxy_form_group_edit($form_id, &$form_state, $group_id) {


  $query = db_select('mcb_proxy_group', 'gr')
      ->fields('gr')
      ->condition('id', $group_id)
      ->execute();
  $result = $query->fetchAll();

  $group = current($result);

  $form['group_id'] = array(
    '#type' => 'hidden',
    '#value' => $group_id,
  );

  $form['group_name'] = array(
    '#type' => 'textfield',
    '#title' => 'Название группы',
    '#default_value' => $group->name,
    '#size' => 40,
    '#maxlength' => 30,
  );

  $form['ldap_dn'] = array(
    '#type' => 'textfield',
    '#title' => 'DN путь в LDAP',
    '#default_value' => $group->ldap_dn,
    '#size' => 40,
    '#maxlength' => 255,
  );

  $form['group_desc'] = array(
    '#type' => 'textarea',
    '#title' => 'Описание группы, чтобы было понятно нахрен она нужна',
    '#rows' => 5,
    '#default_value' => $group->desc,
  );

  $form['vt'] = array(
    '#type' => 'vertical_tabs',
  );

  // Список разрешенных сайтов
  $form['sites_sfa'] = array(
    '#type' => 'fieldset',
    '#title' => 'Разрешенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 10,
  );

  $form['sites_sfa']['sfa'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter allow',
    '#rows' => 15,
    '#default_value' => _mcb_proxy_prepare_regexp($group->sfa),
  );

  // Список запрещенных сайтов
  $form['sites_sfd'] = array(
    '#type' => 'fieldset',
    '#title' => 'Запрещенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 20,
  );

  $form['sites_sfd']['sfd'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter deny',
    '#rows' => 15,
    '#default_value' => _mcb_proxy_prepare_regexp($group->sfd),
  );

  $form['submit'] = array(
    '#value' => 'Обновить',
    '#type' => 'submit',
//    '#attributes' => array('class' => array('btn find')),
  );

  return $form;
}

function mcb_proxy_form_group_edit_validate($form_id, &$form_state) {

  // Проверяем есть лит группа с таким название уже

  $dbg = 0;
  if ($form_state['values']['ldap_dn'] === '') {
    form_set_error('ldap_dn', 'DN Не может быть пустым');
  }
}

/**
 *  Обновляем информацию по группе в DRupal
 * 
 * @param type $form_id
 * @param type $form_state
 */
function mcb_proxy_form_group_edit_submit($form_id, &$form_state) {

  $values = $form_state['values'];
  // Массив полей для записи в таблицу
  $data = array(
    'name' => trim($values['group_name']),
    '`desc`' => $values['group_desc'],
    'ldap_dn' => $values['ldap_dn'],
  );

  $dbg = 0;
  // Обновим описание в LDAP 
  $attributes = array(
    'description' => $values['group_desc'],
  );
  // Проверим есть ли такая группа в LDAP? если нет тогда создадим ее.
  if (!_mcb_proxy_ldap_dnExist($values['ldap_dn'])) {
    $res = mcb_proxy_ldap_groupAddGroup($values['ldap_dn'], $attributes);
  }
  else {
    $res = mcb_proxy_ldap_modify($values['ldap_dn'], $attributes);
  }

  // Обработка строк из текстового поля
  if (isset($values['sfa']) && $values['sfa'] != '') {
    $sfa = _mcb_proxy_sites_filter($values['sfa']);
    $data['sfa'] = $sfa;
  }
  // Обработка строк из текстового поля
  if (isset($values['sfd']) && $values['sfa'] != '') {
    $sfd = _mcb_proxy_sites_filter($values['sfd']);
    $data['sfd'] = $sfd;
  }

  // Пишем в таблицу данные
  $query = db_update('mcb_proxy_group')
      ->fields($data)
      ->condition('id', $values['group_id'])
      ->execute();
  drupal_set_message("Группа " . $values['group_name'] . " обновлена", 'status');

  $form_state['redirect'] = 'mcb/mcb_proxy/admin/groups';
}

/**
 *  Добавление группы во внутреннию систему учета
 * 
 * @return string
 */
function mcb_proxy_form_group_add($form_id, &$form_state) {

  $form['group_name'] = array(
    '#type' => 'textfield',
    '#title' => 'Название группы',
    '#size' => 40,
    '#maxlength' => 30,
  );

  $form['ldap_dn'] = array(
    '#type' => 'textfield',
    '#title' => 'DN путь в LDAP',
    '#default_value' => 'cn=НАЗВАНИЕ ГРУППЫ,ou=Internet_Proxy,ou=ACCESS,o=MCB',
    '#size' => 40,
    '#maxlength' => 255,
    '#disabled' => true,
  );

  $form['group_desc'] = array(
    '#type' => 'textarea',
    '#title' => 'Описание группы, чтобы было понятно нахрен она нужна',
    '#rows' => 5,
  );

  $form['vt'] = array(
    '#type' => 'vertical_tabs',
  );

  // Список разрешенных сайтов
  $form['sites_sfa'] = array(
    '#type' => 'fieldset',
    '#title' => 'Разрешенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 10,
  );

  $form['sites_sfa']['sfa'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter allow',
    '#rows' => 15,
  );

  // Список запрещенных сайтов
  $form['sites_sfd'] = array(
    '#type' => 'fieldset',
    '#title' => 'Запрещенные сайты',
    '#collapsible' => TRUE,
    '#group' => 'vt',
    '#weight' => 20,
  );

  $form['sites_sfd']['sfd'] = array(
    '#type' => 'textarea',
    '#title' => 'Sites filter deny',
    '#rows' => 15,
  );

  $form['submit'] = array(
    '#value' => 'Добавить',
    '#type' => 'submit',
//    '#attributes' => array('class' => array('btn find')),
  );

  return $form;
}

/**
 *  Валидируем форму добавления новой группы во
 * 
 */
function mcb_proxy_form_group_add_validate($form_id, &$form_state) {
  $dbg = 0;
  $values = $form_state['values'];
  $group_name = trim($values['group_name']);
  // Проверяем есть ли такая группа  
  // Учитывая что у нас схема в кодировк utf8_unicode_ci  нам пофиг на регистр при сравнении
  $query = db_select('mcb_proxy_group', 'gr')
      ->fields('gr')
      ->condition('name', $group_name)
      ->execute();
  $count = $query->fetchAll();

  if (count($count) > 0) {
    form_set_error('group_name', 'Такая группа уже есть!');
  }
}

/**
 *  Обрабатываем форму добавления новой группы во
 * 
 */
function mcb_proxy_form_group_add_submit($form_id, &$form_state) {
  $dbg = 0;

  $values = $form_state['values'];

  $group_dn = 'cn=' . $values['group_name'] . ',ou=Internet_Proxy,ou=ACCESS,o=MCB';

  // Массив полей для записи в таблицу
  $data = array(
    'name' => trim($values['group_name']),
    '`desc`' => $values['group_desc'],
    'ldap_dn' => $group_dn,
  );

  $attributes = array(
    'description' => $values['group_desc'],
  );

  $res = mcb_proxy_ldap_groupAddGroup($group_dn, $attributes);

  // Обработка строк из текстового поля
  if (isset($values['sfa']) && $values['sfa'] != '') {
    $sfa = _mcb_proxy_sites_filter($values['sfa']);
    $data['sfa'] = $sfa;
  }
  // Обработка строк из текстового поля
  if (isset($values['sfd']) && $values['sfa'] != '') {
    $sfd = _mcb_proxy_sites_filter($values['sfd']);
    $data['sfd'] = $sfd;
  }
  try {
    // Пишем в таблицу данные
    $query = db_insert('mcb_proxy_group')
        ->fields($data);
    $id = $query->execute();
  }
  catch (Exception $exc) {

    $msg = '<pre>' . $exc->getMessage() . '</pre>';
    $code = "Code: " . $exc->getCode();
    $file = "File: " . $exc->getFile();
    $line = "Line: " . $exc->getLine();
    //$error_inf = "Trace: " . $exc->getTraceAsString();
    dpm($exc->getTraceAsString(), 'ERR-15', 'error');
    $out_str = $msg . '<br>' . $code . '<br>' . $file . '<br>' . $line . '<br>' . $error_inf . '<br>';
    drupal_set_message($out_str, 'error');
  }

  $message = "Группа " . $values['group_name'] . " добавленна!";
  if ($id) {
    drupal_set_message($message, 'status');
  }
}

/**
 * Implements hook_forms();
 */
function mcb_proxy_form_addLdapGroup($form_id, &$form_state) {



  $form['ldapgroup'] = array(
    '#type' => 'textfield',
    '#title' => 'Novel group',
    '#size' => 40,
    '#maxlength' => 30,
  );

  $form['submit'] = array(
    '#value' => 'Добавить',
    '#type' => 'submit',
    '#attributes' => array('class' => array('btn find')),
  );

  return $form;
}

function mcb_proxy_form_addLdapGroup_validate($form_id, &$form_state) {
  $dbg = 1;
}

function mcb_proxy_form_addLdapGroup_submit($form_id, &$form_state) {
  $dbg = 0;
  $group_dn = $form_state['values']['ldapgroup'];
  $group_dn = "cn=$group_dn,ou=Internet_Proxy,ou=ACCESS,o=MCB";
  $res = mcb_proxy_ldap_groupAddGroup($group_dn);
}

/**
 *  Вспомогательная функция выгребает  список групп из Drupal и LDAP
 *  Проводит анализ  какие группы  из Drupal есть в LDAP
 * 
 * @return array   возвращает данные готовые к табличному рендеру 
 */
function _mcb_proxy_groups_list() {
  $dn = 'ou=Internet_Proxy,ou=ACCESS,o=MCB';
  $filter = '(objectClass=groupOfNames)';
  $results = mcb_proxy_ldap_search($dn, $filter);
  $table = _mcb_proxy_ldap_get_group($results);
  //$rows = $table;  


  $query = db_select('mcb_proxy_group', 'gr')
      ->fields('gr')
      ->execute();
  $result = $query->fetchAll();

  foreach ($result as $row) {
    // Позаимствовл от модуля BSS  надо будет переделать
    $noexist = '<div class="blocked"> </div>';
    $exist = '<div class="unblocked"> </div>';
    if (isset($table[$row->name])) {
      $ST = $exist;
    }
    else {
      $ST = $noexist;
    }

    $rows[$row->id] = array(
      $ST,
      '<a href="/mcb/mcb_proxy/admin/group/edit/' . $row->id . '" >' . $row->name . ' </a>',
      $row->desc,
    );
  }

  return $rows;
}

/**
 * Implements hook_forms();
 */
function mcb_proxy_form_groups_list($form_id, &$form_state) {
  $dbg = 1;

  $header = array(
    'eDir?',
    'Group Name',
    'Description',
  );

  // Выводим в табличном виде с пагинацией  
  $form['table'] = array
    (
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => _mcb_proxy_groups_list(),
    '#empty' => 'Группы отсутствуют, необходимо их создать',
  );

  $path = '/mcb/mcb_proxy/admin/group/add';

  $link = l('Добавить новую группу', '/mcb/mcb_proxy/admin/group/add', array(
    'query' => array(
      //'destination' => drupal_get_destination()
      'destination' => current_path()
    ),
    'attributes' => array(
      'title' => 'Добавить новую internet группу'
    )
      )
  );

  // Если это админ прокси, тогда разрешим создание новой группы. Т.к. 
  // ее необходимо конфигурять  в squid.conf
  if (user_access('access protected mcb_proxy_admin')) {
    $form['addgroup'] = array(
      '#value' => 'Добавить новую группу',
      '#type' => 'item',
      '#markup' => '<div class="mcb_proxy addGroup">' . $link . '</div>',
    );
  }
  return $form;
}

/**
 * Implements hook_forms();
 */
function mcb_proxy_form_ldap_search($form_id, &$form_state) {
  $dbg = 0;

  // Find out which step we are on. If $form_state is not set,
  // that means we are beginning. Since the form is rebuilt, we
  // start at 0 in that case and the step is 1 during rebuild.
  $step = isset($form_state['values']) ? (int) $form_state['storage']['step'] : 0;
  $step++;
  $form_state['storage']['step'] = $step;
  $isadmin = user_access('access protected mcb_proxy_admin');
  switch ($step) {
    case 1:
    case 2:

      $collapsed = ($step > 1 ) ? TRUE : FALSE;



      $form['step1'] = array(
        '#type' => 'fieldset',
        '#title' => '1. Задаем фильтры поиска необходимых учеток в LDAP ',
        '#collapsible' => TRUE,
        '#collapsed' => $collapsed,
      );

      //if (!isset($form_state['input'])) {
      //  if (count($form_state['input'])==0) {    
      $form['step1']['dn'] = array(
        '#type' => ($isadmin) ? 'textfield' : 'hidden',
        '#title' => 'LDAP DN search',
        '#size' => 40,
        '#maxlength' => 30,
        '#default_value' => ($isadmin) ? 'ou=HQM,o=MCB' : 'o=MCB',
      );

      $form['step1']['filter'] = array(
        '#type' => ($isadmin) ? 'textarea' : 'hidden',
        '#title' => 'filters',
        //'#default_value' => '(&(objectClass=Person)(objectClass=inetOrgPerson)(loginDisabled=FALSE))',
        '#default_value' => '(&(objectClass=Person)(objectClass=inetOrgPerson))',
        '#rows' => 5,
        '#description' => "<b>Примеры:</b><br>"
        . "Найти насяльников: (&(objectClass=Person)(objectClass=inetOrgPerson)(title=Начальник*))",
      );

      $form['step1']['fios'] = array(
        '#type' => 'textarea',
        '#title' => 'Фамилия Имя отчество на строку',
        //'#default_value' => '(&(objectClass=Person)(objectClass=inetOrgPerson)(loginDisabled=FALSE))',
        //'#default_value' => '',
        '#rows' => 5,
        '#description' => "<b>Построчно</b> указать Ф.И.О искомых людей. Н  апример: </br>"
        . "<em><ul>"
        . "<li>Бочаров Михаил Сергеевич</li>"
        . "<li>Бочаров</li>"
        . "<li>Михаил Сергеевич</li>"
        . "</ul></em>",
      );

      $form['step1']['submit'] = array(
        '#value' => 'Найти!',
        '#type' => 'submit',
        '#attributes' => array('class' => array('btn find')),
      );

      if (!empty($form_state['values'])) {

        $rows = mcb_proxy_ldap_search_users($form_state['values']);
        $header = array(
          array('data' => 'DN',),
          array('data' => 'F.I.O',),
          array('data' => 'title',),
        );

        $collapsed = ($step > 2 ) ? TRUE : FALSE;
        $form['step2'] = array(
          '#type' => 'fieldset',
          '#title' => '2. Выбираем людей которым хотим назначить группы',
          '#collapsible' => TRUE,
          '#collapsed' => $collapsed,
        );

        $form['step2']['users'] = array
          (
          '#type' => 'tableselect',
          '#header' => $header,
          '#options' => $rows,
          '#empty' => 'По данному запросу ничего не найдено.',
        );

        if (count($rows) != 0) {
          $form['step3']['groupselect'] = array(
            '#value' => 'Выбрать группы в которые добавить',
            '#type' => 'submit',
            '#attributes' => array('class' => array('btn find')),
              //'#submit' => array('mcb_proxy_form_search_user_add'),
          );
        }
        else {
          $form_state['storage']['step'] = 1;
        }
      }

      break;
    case 3:

      $dbg = 0;

      // Сохраним выбранных пользователей в промежуточное хранилище
      foreach ($form_state['values']['users'] as $user => $name) {
        if (is_string($name) && $name != "") {
          $form_state['storage']['users'][] = $user;
        }
      }



      $userlist = '';
      foreach ($form_state['storage']['users'] as $user) {
        $userlist .= $user . PHP_EOL;
      }

      $header = array(
        'eDir?',
        'Group Name',
        'Description',
      );

      $form['step3']['userlist'] = array(
        '#type' => 'textarea',
        '#title' => 'Добавить этих персонажей в выбранные ниже группы',
        '#default_value' => $userlist,
        '#disabled' => TRUE,
      );

      // Выводим в табличном виде с пагинацией
      $form['step3']['groups'] = array
        (
        '#type' => 'tableselect',
        '#header' => $header,
        '#options' => _mcb_proxy_groups_list(),
        '#empty' => 'Группы отсутствуют, необходимо их создать',
      );

      $form['step3']['useradd'] = array(
        '#value' => 'Добавить выбранных',
        '#type' => 'submit',
        '#attributes' => array('class' => array('btn find')),
        '#submit' => array('mcb_proxy_form_search_user_add'),
      );
      break;


    default:
      break;
  }


  return $form;
}

function mcb_proxy_form_ldap_search_submit($form_id, &$form_state) {

  $form_state['rebuild'] = TRUE;
//  if ($form_state['storage']['step'] < 4)
//    return;
}

/**
 * SUBMIT формы, добавляем выбранных пользователей в выбранную группу.
 * после чего уничтожаем форму. И делаем редирект.
 * 
 * @param type $form_id
 * @param array $form_state
 */
function mcb_proxy_form_search_user_add($form_id, &$form_state) {
  $dbg = 0;

  $users = $form_state['storage']['users'];

  foreach ($form_state['values']['groups'] as $key => $value) {
    if ((int) $value <> 0) {
      $ids[] = $key;
    }
  }

  // Drupal 7, dynamic query
  $query = db_select('mcb_proxy_group', 'gr')
      ->fields('gr', array('id', 'name', 'ldap_dn'))
      ->condition('id', $ids, 'IN')
      ->execute()
      ->fetchAll();


  foreach ($query as $group) {
    foreach ($users as $user_dn) {
      $dbg = 0;
      $group_dn = $group->ldap_dn;
      $res = mcb_proxy_userAdd($user_dn, $group_dn);
    }
  }

// Clear storage bin to avoid automatic form rebuild that overrides our redirect.
  unset($form_state['storage']);
// Redirect to a thank-you page.
  $form_state['redirect'] = 'mcb/mcb_proxy/admin/ldap/search';
}

/**
 *  Поиск всех пользователей в заданном контексте и фильтре
 * 
 * @param type $values
 */
function mcb_proxy_ldap_search_users(&$values) {
  $dbg = 1;

  // Смотрим заполнили поле ФИО для поиска конкретных персонажей
  $filter_fio = '';
  if ($values['fios'] !== '') {
    $afio = explode("\n", $values['fios']);
    $filter_fio = '(| ';
    foreach ($afio as $fio) {
      if ($fio === '')
        continue;
      $it = explode(" ", $fio);
      $fio_or = '(& ';
      // строим часть конструкции (& (sn=*АнТон*)(sn=*Пятков*))  
      foreach ($it as $val) {
        $fio_or .= "(sn=*" . trim($val) . "*)";
      }
      $fio_or .= " )";
      $filter_fio .= $fio_or;
    }
    $filter_fio .= " )";
  }

  $dn = $values['dn'];
  $filter = $values['filter'];
  // "Выпрямим" фильтр, если его написали в несколько строк
  $patterns = array();
  $patterns[0] = '/[\n\r]/';
  $patterns[1] = '/\s/';
  $replacements = array();
  $replacements[0] = '';
  $replacements[1] = '';
  //$filter = trim(preg_replace($patterns, $replacements, $filter));

  $filter = substr($filter, 0, strlen($filter) - 1) . $filter_fio . ")";

  $result = mcb_proxy_ldap_search($dn, $filter);
  $rows = array();
  if ($result) {
    $count = $result['count'];
    unset($result['count']);

    foreach ($result as $item) {
      $dbg = 0;
      $rows[$item['dn']] = array(
        $item['dn'],
        $item['sn'][0],
        $item['title'][0],
      );
    }
  }

  ksort($rows);

  return $rows;
}
