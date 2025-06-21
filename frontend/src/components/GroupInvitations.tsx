import React, { useEffect, useState, useRef, useCallback } from "react";
import { X, Check, XCircle } from "lucide-react";

interface Invitation {
  id: number;
  group_id: number;
  group_name: string;
  group_description: string;
  inviter_username: string;
  role: string;
  status: string;
  created_at: string;
  expires_at: string;
}

interface Props {
  token: string;
  onAction?: () => void; // callback to refresh group list
}

const GroupInvitations: React.FC<Props> = ({ token, onAction }) => {
  const [invitations, setInvitations] = useState<Invitation[]>([]);
  const [loading, setLoading] = useState(false);
  const [visibleInvitations, setVisibleInvitations] = useState<Invitation[]>([]);
  const [shownInvitationIds, setShownInvitationIds] = useState<Set<number>>(new Set());
  const [processedInvitationIds, setProcessedInvitationIds] = useState<Set<number>>(new Set());
  const isFetchingRef = useRef(false);
  const lastFetchTimeRef = useRef(0);
  const errorCountRef = useRef(0);

  const fetchInvitations = useCallback(async () => {
    // Prevent concurrent fetches
    if (isFetchingRef.current) {
      return;
    }
    
    // Prevent too frequent requests (minimum 5 seconds between requests)
    const now = Date.now();
    if (now - lastFetchTimeRef.current < 5000) {
      return;
    }
    
    isFetchingRef.current = true;
    lastFetchTimeRef.current = now;
    setLoading(true);
    
    try {
      const res = await fetch("/api/invitations", {
        headers: { Authorization: `Bearer ${token}` },
      });
      
      if (res.ok) {
        const data = await res.json();
        const newInvitations = data.data;
        
        // Filter out expired invitations and already processed ones
        const now = new Date();
        const validInvitations = newInvitations.filter((inv: Invitation) => {
          const expiresAt = new Date(inv.expires_at);
          return expiresAt > now && !processedInvitationIds.has(inv.id);
        });
        
        setInvitations(validInvitations);
        
        // Show only invitations that haven't been shown before
        const newInvitationsToShow = validInvitations.filter((inv: Invitation) => 
          !shownInvitationIds.has(inv.id)
        );
        
        if (newInvitationsToShow.length > 0) {
          setVisibleInvitations(prev => [...prev, ...newInvitationsToShow]);
          setShownInvitationIds(prev => new Set([...prev, ...newInvitationsToShow.map((inv: Invitation) => inv.id)]));
        }
        
        // Reset error count on successful fetch
        errorCountRef.current = 0;
      } else {
        console.error('Failed to fetch invitations:', res.status, res.statusText);
        errorCountRef.current++;
        
        // If we get too many errors, stop polling for a while
        if (errorCountRef.current > 5) {
          console.warn('Too many errors fetching invitations, stopping polling temporarily');
          return;
        }
      }
    } catch (error) {
      console.error('Error fetching invitations:', error);
      errorCountRef.current++;
      
      // If we get too many errors, stop polling for a while
      if (errorCountRef.current > 5) {
        console.warn('Too many errors fetching invitations, stopping polling temporarily');
        return;
      }
    } finally {
      setLoading(false);
      isFetchingRef.current = false;
    }
  }, [token, processedInvitationIds, shownInvitationIds]);

  useEffect(() => {
    fetchInvitations();
    // Poll for new invitations every 30 seconds instead of 10
    const interval = setInterval(fetchInvitations, 30000);
    return () => clearInterval(interval);
  }, [fetchInvitations]);

  const handleAction = async (id: number, action: "accept" | "decline") => {
    console.log(`[DEBUG] handleAction: ${action}ing invitation ${id}`);
    try {
      const url = `/api/invitations/${id}/${action}`;
      console.log(`[DEBUG] handleAction: Making request to ${url}`);
      
      const res = await fetch(url, {
        method: "POST",
        headers: { Authorization: `Bearer ${token}` },
      });
      
      console.log(`[DEBUG] handleAction: Response status: ${res.status}`);
      
      if (res.ok) {
        const responseText = await res.text();
        console.log(`[DEBUG] handleAction: Response body: ${responseText}`);
        
        // Mark invitation as processed to prevent re-fetching
        setProcessedInvitationIds(prev => new Set([...prev, id]));
        
        // Remove the invitation from visible popups
        setVisibleInvitations(prev => prev.filter(inv => inv.id !== id));
        setInvitations(prev => prev.filter(inv => inv.id !== id));
        
        if (onAction) onAction();
        
        // Show success message
        if (action === "accept") {
          console.log("Invitation accepted!");
        } else {
          console.log("Invitation declined!");
        }
      } else {
        const errorText = await res.text();
        console.error(`Failed to ${action} invitation:`, res.status, res.statusText, errorText);
      }
    } catch (error) {
      console.error(`Error ${action}ing invitation:`, error);
    }
  };

  const dismissInvitation = (id: number) => {
    setVisibleInvitations(prev => prev.filter(inv => inv.id !== id));
  };

  // Auto-dismiss expired invitations
  useEffect(() => {
    const interval = setInterval(() => {
      const now = new Date();
      setVisibleInvitations(prev => 
        prev.filter(inv => {
          const expiresAt = new Date(inv.expires_at);
          return expiresAt > now;
        })
      );
    }, 60000); // Check every minute

    return () => clearInterval(interval);
  }, []);

  if (visibleInvitations.length === 0) {
    return null; // Don't render anything if no invitations
  }

  return (
    <div className="fixed top-4 right-4 z-50 space-y-2">
      {visibleInvitations.map((inv, index) => {
        const expiresAt = new Date(inv.expires_at);
        const now = new Date();
        const timeLeft = Math.max(0, Math.floor((expiresAt.getTime() - now.getTime()) / (1000 * 60)));
        
        return (
          <div
            key={`${inv.id}-${index}`}
            className="bg-dark-800 border border-cockpit-600/30 rounded-lg p-4 shadow-lg max-w-sm animate-in slide-in-from-right duration-300"
          >
            <div className="flex items-start justify-between mb-3">
              <div className="flex-1">
                <h4 className="text-white font-semibold text-sm mb-1">
                  Group Invitation
                </h4>
                <p className="text-cockpit-400 text-xs">
                  {inv.inviter_username} invited you to join
                </p>
              </div>
              <button
                onClick={() => dismissInvitation(inv.id)}
                className="text-dark-400 hover:text-white transition-colors"
              >
                <X className="w-4 h-4" />
              </button>
            </div>
            
            <div className="mb-3">
              <h5 className="text-white font-medium text-sm mb-1">
                {inv.group_name}
              </h5>
              {inv.group_description && (
                <p className="text-dark-400 text-xs mb-2">
                  {inv.group_description}
                </p>
              )}
              <p className="text-orange-400 text-xs">
                Expires in {timeLeft} minutes
              </p>
            </div>
            
            <div className="flex space-x-2">
              <button
                onClick={() => handleAction(inv.id, "accept")}
                className="flex-1 bg-cockpit-600 hover:bg-cockpit-500 text-white text-xs py-2 px-3 rounded-md transition-colors flex items-center justify-center space-x-1"
              >
                <Check className="w-3 h-3" />
                <span>Accept</span>
              </button>
              <button
                onClick={() => handleAction(inv.id, "decline")}
                className="flex-1 bg-dark-700 hover:bg-dark-600 text-white text-xs py-2 px-3 rounded-md transition-colors flex items-center justify-center space-x-1"
              >
                <XCircle className="w-3 h-3" />
                <span>Decline</span>
              </button>
            </div>
          </div>
        );
      })}
    </div>
  );
};

export default GroupInvitations; 