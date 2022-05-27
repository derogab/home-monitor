import React from "react";
import ReactDOM from "react-dom";
import { BrowserRouter, Route, Switch, Redirect } from "react-router-dom";

import "@fortawesome/fontawesome-free/css/all.min.css";
import "assets/styles/tailwind.css";

// components

import Navbar from "components/Navbars/Navbar.js";
import Sidebar from "components/Sidebar/Sidebar.js";
import HeaderStats from "components/Headers/HeaderStats.js";
import FooterAdmin from "components/Footers/FooterAdmin.js";

// views

import Dashboard from "views/Dashboard.js";
import Settings from "views/Settings.js";
import Tables from "views/Tables.js";

ReactDOM.render(
  <BrowserRouter>
    <Sidebar />
      <div className="relative md:ml-64 bg-blueGray-100">
        <Navbar />
        {/* Header */}
        <HeaderStats />
        <div className="px-4 md:px-10 mx-auto w-full -m-24">
          <Switch>
            <Route path="/dashboard" exact component={Dashboard} />
            <Route path="/settings" exact component={Settings} />
            <Route path="/tables" exact component={Tables} />
            <Redirect from="/" to="/dashboard" />
          </Switch>
          <FooterAdmin />
        </div>
      </div>
  </BrowserRouter>,
  document.getElementById("root")
);
